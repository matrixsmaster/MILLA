#include "mviewer.h"
#include "ui_mviewer.h"
#include "searchform.h"
#include "aboutbox.h"
#include "splashscreen.h"

MViewer::MViewer(QWidget *parent) : QMainWindow(parent)
{
    ui = new Ui::MViewer();
    ui->setupUi(this); //create the actual form

    //Prepare the splash sccreen
    SplashScreen* sscp = new SplashScreen();
    sscp->show();
    sscp->postShow();

    //Load the database
    if (!db.initDatabase([sscp] (double p) { return sscp->setProgress(p); } )) {
        QMessageBox::critical(this, tr("Fatal error"), tr("Unable to initialize database"));
        qDebug() << "FATAL: Unable to initialize database " DB_FILEPATH;

        //in case of failure, we can't proceed, so we use the view timer as auto-close event generator
        connect(&view_timer,&QTimer::timeout,this,[] { QApplication::exit(1); });
        view_timer.start(2);

        //remove splash screen
        sscp->hide();
        delete sscp;
        return;
    }

    //Init globals
    loader = new MImageLoader(&plugins,this);
    loadingMovie = new QMovie(":/loading_icon.gif",QByteArray(),this);
    a_story = new MImageOps(loader,this);

    progressBar = new QProgressBar(this);
    progressBar->setTextVisible(false);
    ui->statusBar->addPermanentWidget(progressBar);

    stopButton = new QPushButton(this);
    stopButton->setText("stop");
    stopButton->setEnabled(false);
    ui->statusBar->addPermanentWidget(stopButton);

    //Callback function for normal view timer
    connect(&view_timer,&QTimer::timeout,this,[this] {
        if (progressBar->value() < 100)
            progressBar->setValue(progressBar->value()+1);
        else {
            view_timer.stop();
            ui->lcdNumber->display((double)incViews());
        }
    });
    //Callback function for long processing interrupt button
    connect(stopButton,&QPushButton::clicked,this,[this] { stop_flag = true; });

    //Callback function for progressbar for external processing facilities
    prog_callback = ([this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

    //Connect the Star Labels to rating function
    connect(ui->star_1,&StarLabel::clicked,this,[this] { changedStars(1); });
    connect(ui->star_2,&StarLabel::clicked,this,[this] { changedStars(2); });
    connect(ui->star_3,&StarLabel::clicked,this,[this] { changedStars(3); });
    connect(ui->star_4,&StarLabel::clicked,this,[this] { changedStars(4); });
    connect(ui->star_5,&StarLabel::clicked,this,[this] { changedStars(5); });

    //Enable receiving the MouseMove events for every widget in the main window
    enableMouseMoveEvents(children());

    //Connect plugins subsystem to main window
    plugins.setViewerContext(MillaPluginContext({ &current_l,ui->scrollArea_2,ui->label_2,this }));
    //List all loaded plugins
    plugins.addPluginsToMenu(*(ui->menuPlugins),prog_callback);

    //Create the "short-term memory" tab
    createMemoryTab();

    //Create the Directory tab
    ui->dirList->setModel(new MDirectoryModel(loader,ui->dirList));

    //Enable events filter for left pane (for selection)
    ui->scrollArea->installEventFilter(this);

    //Enable events filter for labels (for copying file names)
    ui->label_3->installEventFilter(this);
    ui->label_4->installEventFilter(this);

    //Reset form
    cleanUp();
    updateTags();
    updateRecents();
    updateWindowLayout("window");
    ui->mainToolBar->hide();

    //Update status bar with pending message(s)
    status_pending = "[" + db.getDBInfoString() + "]";
    ui->statusBar->showMessage(status_pending);
    status_pending += " Current dir: ";

    //Remove splash screen
    sscp->hide();
    delete sscp;
}

MViewer::~MViewer()
{
    updateWindowLayout("window",true);
    delete ui;
}

void MViewer::createMemoryTab()
{
    MMemoryModel* mmm = new MMemoryModel(loader,ui->memList);
    ui->memList->setModel(mmm);
    //Create the menu actions associated with each of memory slots
    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        QAction* a = ui->menuShort_term_memory->addAction(QString::asprintf("Slot %d",i+1));
        if (!a) break;
        a->setShortcut(QKeySequence(QString::asprintf("F%d",i+1)));
        connect(a,&QAction::triggered,this,[i,mmm,this] {
            if (this->current_l.valid && !this->current_l.generated) {
                mmm->setSlot(i,this->current_l);
                ui->tabWidget->setCurrentIndex(MVTAB_MEMORY);
            }
        });
    }
    //Add "erase" actions
    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        QAction* a = ui->menuShort_term_memory->addAction(QString::asprintf("Erase slot %d",i+1));
        if (!a) break;
        a->setShortcut(QKeySequence(QString::asprintf("Ctrl+F%d",i+1)));
        connect(a,&QAction::triggered,this,[i,mmm] { mmm->eraseSlot(i); });
    }
    //Connect the memory tab selection model to right image pane
    connect(ui->memList->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->memList->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });
}

void MViewer::updateWindowLayout(const QString &name, bool save)
{
    db.setWinTableName(name);
    if (save) {
        db.updateWindowGeometryAndState(saveGeometry(),saveState());
        db.updateViewerState(children());
    } else {
        restoreGeometry(db.getWindowGeometryOrState(true));
        restoreState(db.getWindowGeometryOrState(false));
        db.restoreViewerState(children());
    }
}

void MViewer::cleanUp()
{
    extra_cache.clear();

    current_l = MImageListRecord();
    current_r = MImageListRecord();

    history.files.clear();
    history.cur = history.files.begin();
    jump_buf.clear();
    jump_idx = 0;

    if (ui->listView->model()) ui->listView->model()->deleteLater();
    if (ui->listView_2->model()) ui->listView_2->model()->deleteLater();
    if (ui->listView_3->model()) ui->listView_3->model()->deleteLater();

    ui->label->setPixmap(QPixmap());
    ui->label_2->setPixmap(QPixmap());
    ui->label_3->clear();
    ui->label_4->clear();
    ui->plainTextEdit->clear();

    updateTags();
    updateStars();
    ui->lcdNumber->display(0);
    ui->lcdNumber_2->display(0);
    ui->statusBar->clearMessage();
    progressBar->setValue(0);
}

void MViewer::addTag(QString const &tg, unsigned key, bool check)
{
    ui->listWidget->addItem(tg);
    QListWidgetItem* i = ui->listWidget->item(ui->listWidget->count()-1);
    if (!i) return;

    if (ui->radio_search->isChecked())
        i->setFlags(i->flags() | Qt::ItemIsUserTristate);
    else
        i->setFlags(i->flags() | Qt::ItemIsUserCheckable);

    Qt::CheckState checkst = check? Qt::Checked : Qt::Unchecked;
    i->setCheckState(checkst);
    tags_cache[tg] = std::pair<unsigned,Qt::CheckState>(key,checkst);
}

void MViewer::updateTags(QString const &fn)
{
    ui->listWidget->clear();
    tags_cache.clear();

    MTagsCheckList l = db.getFileTags(fn);
    for (auto &i : l)
        addTag(std::get<0>(i),std::get<1>(i),std::get<2>(i));
}

void MViewer::updateFileTags()
{
    if (current_l.valid) db.updateFileTags(current_l.filename,tags_cache);
}

void MViewer::updateStars(QString const &fn)
{
    int n = db.getFileRating(fn);
    ui->star_1->setStarActivated(n > 0);
    ui->star_2->setStarActivated(n > 1);
    ui->star_3->setStarActivated(n > 2);
    ui->star_4->setStarActivated(n > 3);
    ui->star_5->setStarActivated(n > 4);
}

void MViewer::changedStars(int n)
{
    if (!current_l.valid) return;
    db.updateFileRating(current_l.filename,n);
    updateStars(current_l.filename);
}

void MViewer::prepareLongProcessing(bool finish)
{
    progressBar->setValue(finish? 100 : 0);
    stopButton->setEnabled(!finish);
    stop_flag = false;

    if (finish) {
        //QApplication::restoreOverrideCursor();
        loadingMovie->stop();
        if (loadingLabel) {
            ui->statusBar->removeWidget(loadingLabel);
            delete loadingLabel;
            loadingLabel = nullptr;
        }

    } else {
        //QApplication::setOverrideCursor(Qt::WaitCursor);
        loadingLabel = new QLabel(this);
        loadingLabel->setMovie(loadingMovie);
        ui->statusBar->insertWidget(1,loadingLabel);
        loadingMovie->jumpToFrame(0);
        loadingMovie->start();
    }
}

void MViewer::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit->text().contains(',')) return;

    QString ntag = ui->lineEdit->text().trimmed();
    if (ntag.isEmpty()) return;
    ntag = ntag.replace('\"','\'');

    unsigned key;
    if (db.insertTag(ntag,key)) {
        addTag(ntag,key);
        ui->lineEdit->clear();
    }
}

bool MViewer::createStatRecord(QString const &fn, bool cache_global)
{
    if (db.isStatRecordExists(fn)) {
        qDebug() << "ALERT: createStatRecord() called for known file";
        return false;
    }

    MImageExtras ext = getExtraCacheLine(fn,true,cache_global);
    if (!ext.valid) {
        qDebug() << "ALERT: invalid extra data returned for " << fn;
        return false;
    }

    bool ok = db.updateStatRecord(fn,ext,false);
    qDebug() << "[db] Creating new statistics record: " << ok;

    checkExtraCache();
    return true;
}

void MViewer::checkExtraCache()
{
    if (extra_cache.size() > MILLA_EXTRA_CACHE_SIZE) extra_cache.clear();
}

MImageExtras MViewer::getExtraCacheLine(QString const &fn, bool forceload, bool ignore_thumbs)
{
    //is it already cached?
    if (extra_cache.count(fn))
        return extra_cache[fn];

    //let's try to query DB
    MImageExtras res = db.getExtrasFromDB(fn);
    if (res.valid) {
        extra_cache[fn] = res; //cache query result
        return res;
    }

    //retreive original image
    QPixmap org;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm && !ignore_thumbs) {
        size_t idx = 0;
        for (auto &i : ptm->GetAllImages()) {
            if (i.filename == fn) {
                if (forceload) ptm->LoadUp(idx);
                if (i.loaded) org = i.picture;
                break;
            }
            idx++;
        }
    } else if (forceload)
        org.load(fn);

    //check retrieval
    if (org.isNull()) return res;

    //compute metadata and cache up results
    res = mCV.collectImageExtraData(fn,org);
    if (res.valid) extra_cache[fn] = res;

    return res;
}

void MViewer::updateModelThumbnailSettings(ThumbnailModel* ptr, bool purelist)
{
    ptr->setShortenFilenames(!purelist);
    ui->listView->setViewMode(purelist? QListView::ListMode : QListView::IconMode);
    ui->listView->setFlow(purelist? QListView::TopToBottom : QListView::LeftToRight);
    ui->listView->setWrapping(!purelist);
    ui->listView->setSpacing(purelist? 5:10);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MViewer::showImageList(QStringList const &lst)
{
    cleanUp(); //remove everything
    prepareLongProcessing(true); //reset progress bar after thumbnails loading

    //prepare new selection model
    ThumbnailModel* ptr = new ThumbnailModel(lst,[this] (auto v) {
        //progress bar callback
        this->progressBar->setValue(floor(v));
        QCoreApplication::processEvents();
        return true;
    },
    loader,
    ui->listView);

    //fill in settings to correctly display this selection model
    ui->listView->setModel(ptr);
    updateModelThumbnailSettings(ptr,!(ui->actionThumbnails_cloud->isChecked()));

    //connect events to selection model
    connect(ui->listView->selectionModel(),&QItemSelectionModel::selectionChanged,[this] { showSelectedImage(); });
    connect(ui->listView,&QListView::customContextMenuRequested,this,[this] {
        current_r = ui->listView->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });

    //update status bar
    ui->statusBar->showMessage(status_pending+QString::asprintf("%d images",lst.size()));
    status_pending.clear();

    //update other fields
    updateRecents();
    on_actionDescending_triggered(); //renew sorting
    //on_pushButton_9_clicked(); //clear story (?)
}

void MViewer::showSelectedImage()
{
    //check if any image is actually selected in current selection model
    if (ui->listView->selectionModel()->selectedIndexes().isEmpty()) return;

    //retrieve the image index, load it and update timestamp
    QModelIndex idx = ui->listView->selectionModel()->selectedIndexes().first();
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->LoadUp(idx.row());
    else return;
    ptm->touch(idx);

    //update globals
    current_l = idx.data(MImageListModel::FullDataRole).value<MImageListRecord>();
    face_idx = face_idx_roi = -1;

    //update image and associated data fields
    scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1.f);
    leftImageMetaUpdate();
    ui->lineEdit_2->clear();

    //garbage collection
    checkExtraCache();

    //play special file
    if (ui->actionAutoplay_special_files->isChecked())
        qDebug() << "Opening file as a special file format: " << plugins.openFileFormat(current_l.filename);

    //check if some time-consuming process is underway, don't use the view timer
    if (!stopButton->isEnabled()) {
        progressBar->setValue(0);
//        ui->statusBar->showMessage("");
        view_timer.start(MILLA_VIEW_TIMER);
    }
}

void MViewer::scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, QLabel* inflbl, double factor)
{
    if (!rec.valid) return;

    scaleFactor *= factor;
    if (scaleFactor <= FLT_EPSILON) scaleFactor = 1;
    //qDebug() << "Scale factor =" << scaleFactor;

    scrl->setWidgetResizable(false);
    lbl->setPixmap(QPixmap());
    lbl->setText("");
    lbl->updateGeometry();
    scrl->updateGeometry();
    scrl->setWidgetResizable(true);
    inflbl->setText(QString::asprintf("%s: %d x %d",
                                      rec.fnshort.left(MILLA_MAXSHORTLENGTH).toStdString().c_str(),
                                      rec.picture.size().width(),
                                      rec.picture.size().height()));

    if (ui->actionFit->isChecked())
        lbl->setPixmap(rec.picture.scaled(lbl->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));

    else {
        QSize nsz = rec.picture.size() * scaleFactor;
        MImageExtras extr = getExtraCacheLine(rec.filename);

        if (ui->actionShow_faces->isChecked() || ui->actionCenter_on_face->isChecked()) {
            QRect vbd;
            lbl->setPixmap(CVHelper::drawROIs(rec.picture,vbd,extr,!ui->actionShow_faces->isChecked(),face_idx_roi));

            if (ui->actionCenter_on_face->isChecked()) {
                //it takes some time to actually show the image, so wait until next GUI cycle
                QCoreApplication::processEvents();
                //now center on face
                scrl->ensureVisible(vbd.x(),vbd.y(),vbd.width(),vbd.height());
            }

        } else
            lbl->setPixmap(rec.picture.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }

    if (lbl == ui->label) selection_fsm = 0; //invalidate selection for left panel
}

unsigned MViewer::incViews(bool left)
{
    if (    (left && (!current_l.valid || current_l.generated)) ||
            (!left && (!current_r.valid || current_r.generated))    ) return 0;

    QString fn = (left? current_l : current_r).filename;
    if (fn.isEmpty()) return 0;
    qDebug() << "Incrementing views counter for " << fn;

    bool ok;
    unsigned v = db.getFileViews(fn,ok);
    if (!ok) {
        //first time watching, let's create a record
        if (!createStatRecord(fn)) return 0;
    }

    //auto-increment kudos for highly-rated image before incrementing the views counter
    if (!left && v && v == (unsigned)db.updateFileKudos(current_r.filename,0) && ui->actionAuto_inc_kudos_for_right_image->isChecked())
        db.updateFileKudos(current_r.filename,1);

    //increment views counter
    if (!db.updateFileViews(fn,++v)) return 0;

    if (left) {
        //add to history
        bool add = history.files.empty() || (history.cur == history.files.end());
        if (!add) add = !history.files.contains(current_l.filename);
        if (add) {
            history.files.push_back(current_l.filename);
            history.cur = history.files.end();
        }
    }

    return v;
}

void MViewer::leftImageMetaUpdate()
{
    if (current_l.valid && !current_l.generated) {
        updateTags(current_l.filename);
        updateStars(current_l.filename);

        if (ui->actionShow_linked_image->isChecked())
            displayLinkedImages(current_l.filename);

        bool ok;
        ui->lcdNumber->display((double)db.getFileViews(current_l.filename,ok));
        if (ok) ui->plainTextEdit->setPlainText(db.getFileNotes(current_l.filename));
        else ui->plainTextEdit->clear();

        if (!ui->plainTextEdit->toPlainText().isEmpty())
            ui->tabWidget->setCurrentIndex(MVTAB_NOTES);
    }
    kudos(current_l,0);
    ui->radio_settags->setChecked(true);
}

void MViewer::processArguments()
{
    QStringList args = QApplication::arguments();
    if (args.size() < 2) {
        qDebug() << "[DEBUG] Loading most recent dir " << db.getMostRecentDir();
        showImageList(loader->open(db.getMostRecentDir()));
        return;
    }
    args.erase(args.begin());

    loader->clearList();
    for (auto &i : args) loader->append(i,(args.size() > 1));
    showImageList(loader->getList());

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) {
        QString canon;
        if (loader->isLoadableFile(args.front(),&canon))
            ui->listView->setCurrentIndex(ptm->getRecordIndex(canon));
        else
            ui->listView->setCurrentIndex(ptm->getRecordIndex(0));
    }
}

void MViewer::on_actionOpen_triggered()
{
    QString flt(MILLA_OPEN_FILE);
    flt += " (*." + loader->getSupported().join(" *.") + ")";

    QString fn = QFileDialog::getOpenFileName(this,tr("Open image and directory"),"",flt);
    if (fn.isEmpty()) return;
    showImageList(loader->open(fn));

    QString canon;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm && loader->isLoadableFile(fn,&canon)) ui->listView->setCurrentIndex(ptm->getRecordIndex(canon));
}

void MViewer::on_actionOpen_list_triggered()
{
    QString flt(MILLA_OPEN_LIST);
    QStringList _l = MILLA_LIST_FORMATS;
    flt += " (*." + _l.join(" *.") + ")";

    showImageList(loader->open(QFileDialog::getOpenFileName(this,tr("Open list of images"),"",flt)));
}

void MViewer::on_actionFit_triggered()
{
    if (!ui->actionFit->isChecked()) scaleFactor = 1;
    scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
}

void MViewer::on_actionMatch_triggered()
{
    if (!current_l.valid) return;

    MImageExtras orig = getExtraCacheLine(current_l.filename);
    if (!orig.valid) {
        if (current_l.generated)
            orig = mCV.collectImageExtraData(QString(),current_l.picture);
        else
            return;
    }

    //destroy model immediately, to prevent it from loading more images (if background loading is activated)
    SResultModel* mdl = static_cast<SResultModel*>(ui->listView_2->model());
    ui->listView_2->setModel(nullptr);
    delete mdl;

    MMatcher match(orig,current_l.filename,MILLA_MAXMATCH_RESULTS);
    QStringList lst;

    if (ui->actionGlobal_search->isChecked()) {
        prepareLongProcessing();
        lst = match.GlobalMatcher(prog_callback);
        prepareLongProcessing(true);

    } else {
        ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
        if (!ptm) return;
        lst = match.LocalMatcher(ptm->GetAllImages(),[this] (auto s) {
            this->checkExtraCache();
            return this->getExtraCacheLine(s);
        },prog_callback);
    }

    searchResults(lst);
}

void MViewer::on_actionLoad_everything_slow_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    double passed = 0, spd = 0, est, prg = 0, all = (double)(ptm->GetAllImages().size());
    double dp = 100.f / all;
    size_t k = 0;

    prepareLongProcessing();

    using namespace std::chrono;
    auto start = steady_clock::now();

    for (auto &i : ptm->GetAllImages()) {
        QCoreApplication::processEvents();
        if (stop_flag) break;
        createStatRecord(i.filename); //ignore result

        prg += dp;
        passed = (duration_cast<duration<double>>(steady_clock::now() - start)).count();
        spd = (double)(k++) / ((passed < FLT_EPSILON)? FLT_EPSILON : passed);
        est = (all - k) / spd;

        progressBar->setValue(floor(prg));
        ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %s; Processed: %lu; Speed: %.2f img/sec; Remaining: %s",
                                                     all,
                                                     DBHelper::timePrinter(passed).toStdString().c_str(),
                                                     k,spd,
                                                     DBHelper::timePrinter(est).toStdString().c_str()));
    }

    ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %s; Speed: %.2f img/sec. %s.",
                                                 all,
                                                 DBHelper::timePrinter(passed).toStdString().c_str(),
                                                 spd,
                                                 (stop_flag?"Cancelled by user":"Finished")));
    prepareLongProcessing(true);
}

void MViewer::on_listWidget_itemClicked(QListWidgetItem *item)
{
    if (!tags_cache.count(item->text())) {
        qDebug() << "ALERT: unknown tag " << item->text();
        return;
    }

    Qt::CheckState was = tags_cache[item->text()].second;
    Qt::CheckState now = item->checkState();
    if (was == now) return;

    if (ui->radio_search->isChecked()) {
        //search by tag instead of change tags
        tags_cache[item->text()].second = now;

        ThumbnailModel* ptm;
        if (ui->actionGlobal_search->isChecked()) ptm = nullptr;
        else {
            ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
            if (!ptm) return;
        }

        searchResults(db.tagSearch(tags_cache,(ptm? &(ptm->GetAllImages()):nullptr),MILLA_MAXTAG_RESULTS));
        return;
    }

    if (!current_l.valid || current_l.generated) {
        //revert change
        item->setCheckState(Qt::Unchecked);
        return;
    }

    if (db.updateTags(item->text(),now))
        tags_cache[item->text()].second = now;
    updateFileTags();
}

void MViewer::on_radio_search_toggled(bool checked)
{
    if (checked) updateTags();
}

void MViewer::on_radio_settags_toggled(bool checked)
{
    if (checked) {
        if (current_l.valid) updateTags(current_l.filename);
        else updateTags();
    }
}

void MViewer::resultsPresentation(QStringList lst, QListView* view, int tabIndex)
{
    if (view->model()) {
        qDebug() << "Old model scheduled for removal";
        view->model()->deleteLater();
    }
    if (lst.isEmpty()) return;

    QList<MImageListRecord> out;
    for (auto &i : lst) {
        MImageListRecord r;
        r.filename = i;
        out.push_back(r);
    }

    view->setModel(new SResultModel(out,loader,view));
    view->setViewMode(QListView::ListMode);
    view->setFlow(QListView::LeftToRight);
    view->setWrapping(false);
    view->setWordWrap(true);

    ui->tabWidget->setCurrentIndex(tabIndex);
}

void MViewer::searchResults(QStringList lst)
{
    ui->statusBar->showMessage(QString::asprintf("%i images found",lst.count()));
    resultsPresentation(lst,ui->listView_2,MVTAB_RESULTS);

    connect(ui->listView_2->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        MImageListRecord _r = ui->listView_2->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        if (ui->actionLeft_image->isChecked()) {
            current_l = _r;
            scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
            leftImageMetaUpdate();
            ui->lcdNumber->display((double)incViews());
        } else {
            current_r = _r;
            scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
            incViews(false);
        }
    });
}

void MViewer::on_actionJump_to_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    bool ok;
    QString fn = QInputDialog::getText(this,tr("Jump to file"),tr("File name or full path"),QLineEdit::Normal,QString(),&ok);

    if (ok) {
        jump_buf = fn;
        jump_idx = 0;
        ui->listView->setCurrentIndex(ptm->getRecordIndex(fn,true,&jump_idx));
    }
}

void MViewer::on_actionRefine_search_triggered()
{
    SearchForm frm;
    if (search_cnt) frm.set_data(last_search);
    if (!frm.exec()) return;

    SearchFormData flt = frm.getSearchData();
    if (current_l.valid) {
        if (flt.similar > 0) {
            MImageExtras ex = getExtraCacheLine(current_l.filename);
            if (ex.valid)
                flt.similar_to = ex.hist;
            else if (current_l.generated)
                flt.similar_to = CVHelper::getHist(current_l.picture);
            else
                flt.similar = 0;
        }
    } else
        flt.similar = 0;

    last_search = flt;
    search_cnt++;

    bool glob = false;
    bool cont = false;
    MImageListModel* ptm = NULL;
    switch (flt.scope) {
    case SRSCP_NEW:
        ptm = dynamic_cast<MImageListModel*>(ui->listView->model());
        glob = ui->actionGlobal_search->isChecked();
        break;
    case SRSCP_FOUND:
        ptm = dynamic_cast<MImageListModel*>(ui->listView_2->model());
        break;
    case SRSCP_LINKS:
        ptm = dynamic_cast<MImageListModel*>(ui->listView_3->model());
        break;
    case SRSCP_CONTINUE:
        cont = true;
        break;
    }

    if (!cont) {
        if (!ptm || glob) {
            QList<MImageListRecord> lst;
            QStringList fls =  db.getAllFiles();
            for (auto &i : fls) {
                MImageListRecord r;
                QFileInfo fi(i);
                r.filename = i;
                r.filechanged = fi.lastModified().toTime_t();
                lst.push_back(r);
            }
            db.initParametricSearch(lst);
        } else if (ptm)
            db.initParametricSearch(ptm->GetAllImages());
    }

    QStringList res;
    prepareLongProcessing();
    res = db.doParametricSearch(flt,prog_callback);
    prepareLongProcessing(true);

    if (res.empty())
        QMessageBox::information(this,tr("Search"),tr("Nothing found. Try to relax the search parameters."));
    else
        searchResults(res);
    ui->actionContinue_search->setEnabled(!res.empty());
}

void MViewer::on_actionSwap_images_triggered()
{
    MImageListRecord tmp(current_l);
    current_l = current_r;
    current_r = tmp;
    scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
    leftImageMetaUpdate();
}

void MViewer::on_actionClear_results_triggered()
{
    if (ui->listView_2->model()) ui->listView_2->model()->deleteLater();
    ui->actionContinue_search->setEnabled(false);
    search_cnt = 0;
}

void MViewer::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MViewer::on_actionLink_left_to_right_triggered()
{
    linkageAction(true);
}

void MViewer::on_actionLink_bidirectional_triggered()
{
    on_actionLink_left_to_right_triggered();
    on_actionSwap_images_triggered();
    on_actionLink_left_to_right_triggered();
    on_actionSwap_images_triggered();
}

void MViewer::displayLinkedImages(QString const &fn)
{
    QStringList out = db.getLinkedImages(fn,false);
    if (ui->actionShow_reverse_links->isChecked())
        out += db.getLinkedImages(fn,true);

    if (ui->actionExplore_links_tree->isChecked()) {
        QSet<QString> inset, outset = out.toSet();
        int c = 0;
        do {
            for (auto &i : outset) {
                QByteArray _sha = db.getSHAbyFile(i);
                if (_sha.isEmpty()) continue;
                QStringList _tmp = db.getLinkedImages(_sha,false);
                if (ui->actionShow_reverse_links->isChecked())
                    _tmp += db.getLinkedImages(_sha,true);
                inset.unite(_tmp.toSet());
            }
//            qDebug() << inset;
            int p = outset.size();
            outset.unite(inset);
            inset.clear();
            c = outset.size() - p;
            qDebug() << "Link tree: c = " << c;
        } while (c > 0);
        out = QStringList::fromSet(outset);
    }

    ui->statusBar->showMessage(QString::asprintf("%i linked images",out.count()));
    resultsPresentation(out,ui->listView_3,MVTAB_LINKS);
    connect(ui->listView_3->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->listView_3->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });
}

void MViewer::on_actionAbout_triggered()
{
    AboutBox box;
    box.exec();
}

void MViewer::on_pushButton_2_clicked()
{
    if (current_l.valid && !current_l.generated)
        db.updateFileNotes(current_l.filename,ui->plainTextEdit->toPlainText().replace('\"','\''));
}

void MViewer::on_actionKudos_to_left_image_triggered()
{
    kudos(current_l,1);
}

void MViewer::on_actionKudos_to_right_image_triggered()
{
    kudos(current_r,1);
}

void MViewer::kudos(MImageListRecord const &to, int delta)
{
    ui->lcdNumber_2->display(0);
    if (!to.valid || to.generated) return;
    ui->lcdNumber_2->display(db.updateFileKudos(to.filename,delta));
}

void MViewer::on_actionLeft_image_triggered()
{
    ui->actionLeft_image->setChecked(true);
    ui->actionRight_image->setChecked(false);
}

void MViewer::on_actionRight_image_triggered()
{
    ui->actionLeft_image->setChecked(false);
    ui->actionRight_image->setChecked(true);
}

void MViewer::on_actionClear_image_cache_triggered()
{
    extra_cache.clear();
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->clearCache();
}

void MViewer::on_actionThumbnails_cloud_changed()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) updateModelThumbnailSettings(ptm,!(ui->actionThumbnails_cloud->isChecked()));
}

void MViewer::selectIEFileDialog(bool import)
{
    //show import/export form and determine search filter values
    ExportForm frm(import);
    if (!frm.exec()) return; //user cancelled the request
    ExportFormData s = frm.getExportData();

    //request filename to save or load from
    QString fileName = import?
                QFileDialog::getOpenFileName(this, tr("Import from"), "", tr("Text Files [txt,csv] (*.txt *.csv)")) :
                QFileDialog::getSaveFileName(this, tr("Export to"), "", tr("Text Files [txt,csv] (*.txt *.csv)"));
    if (fileName.isEmpty()) return;

    //auto-complete file extension in case of exporting table sheet
    if ((!import) && (fileName.right(4).toUpper() != ".CSV" && fileName.right(4).toUpper() != ".TXT"))
        fileName += ".csv";

    //open file andd set up the stream
    QFile fl(fileName);
    if (!fl.open(QIODevice::Text | (import? QIODevice::ReadOnly : QIODevice::WriteOnly))) {
        qDebug() << "ALERT: unable to gain access to " << fileName;
        return;
    }
    QTextStream strm(&fl);

    //if we want to export currently loaded files only, prepare model's pointer
    ThumbnailModel* ptm = s.loaded_only? dynamic_cast<ThumbnailModel*>(ui->listView->model()) : nullptr;

    updateTags(); //re-create tags cache without any checked items
    ui->frame_6->setEnabled(false); //and disable all tag-related stuff temporarily to prevent data loss caused by user's actions
    ui->pushButton->setEnabled(false);

    //create importer
    MImpExpModule mod(&tags_cache,(ptm? &(ptm->GetAllImages()) : nullptr));
    mod.setProgressBar(prog_callback);

    //and start the process
    prepareLongProcessing();
    bool ok = import?
                mod.dataImport(s,strm,[this,s] (auto fn) { return this->createStatRecord(fn,!s.loaded_only); }) :
                mod.dataExport(s,strm);

    //restore tags view
    if (current_l.valid) updateTags(current_l.filename);
    else updateTags();
    ui->frame_6->setEnabled(true);
    ui->pushButton->setEnabled(true);

    //fold up
    prepareLongProcessing(true);
    fl.close();
    qDebug() << "[db] " << (import? "Import":"Export") << " data: " << ok;

    if (!ok) QMessageBox::critical(this, tr("Import/Export error"), tr("Unable to finish operation"));
    else ui->statusBar->showMessage("Import/export operation finished");
}

void MViewer::on_actionExport_data_triggered()
{
    selectIEFileDialog(false);
}

void MViewer::on_actionImport_data_triggered()
{
    selectIEFileDialog(true);
}

void MViewer::on_actionUpdate_thumbnails_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    prepareLongProcessing();

    int m = ptm->GetAllImages().size();
    for (int i = 0; i < m && !stop_flag; i++) {
        progressBar->setValue(floor((double)i / (double)m * 100.f));
        ptm->LoadUp(i,true);
        QCoreApplication::processEvents();
    }

    prepareLongProcessing(true);
}

void MViewer::updateThumbnailsOrder(ThumbnailModel::ThumbnailModelSort ord, bool desc)
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    switch (ord) {
    case ThumbnailModel::SortByNameAsc:
        if (desc) ord = ThumbnailModel::SortByNameDesc;
        break;

    case ThumbnailModel::SortByNameDesc:
        if (!desc) ord = ThumbnailModel::SortByNameAsc;
        break;

    case ThumbnailModel::SortByTimeAsc:
        if (desc) ord = ThumbnailModel::SortByTimeDesc;
        break;

    case ThumbnailModel::SortByTimeDesc:
        if (!desc) ord = ThumbnailModel::SortByTimeAsc;
        break;

    default: break;
    }

    ptm->sortBy(ord);
}

void MViewer::on_action_None_triggered()
{
    ui->action_None->setChecked(true);
    ui->actionBy_name->setChecked(false);
    ui->actionBy_time->setChecked(false);
    updateThumbnailsOrder(ThumbnailModel::NoSort,false);
}

void MViewer::on_actionBy_name_triggered()
{
    ui->action_None->setChecked(false);
    ui->actionBy_name->setChecked(true);
    ui->actionBy_time->setChecked(false);
    updateThumbnailsOrder(ThumbnailModel::SortByNameAsc,ui->actionDescending->isChecked());
}

void MViewer::on_actionBy_time_triggered()
{
    ui->action_None->setChecked(false);
    ui->actionBy_name->setChecked(false);
    ui->actionBy_time->setChecked(true);
    updateThumbnailsOrder(ThumbnailModel::SortByTimeAsc,ui->actionDescending->isChecked());
}

void MViewer::on_actionDescending_triggered()
{
    ThumbnailModel::ThumbnailModelSort ord = ThumbnailModel::NoSort;
    if (ui->actionBy_name->isChecked()) ord = ThumbnailModel::SortByNameAsc;
    else if (ui->actionBy_time->isChecked()) ord = ThumbnailModel::SortByTimeAsc;

    updateThumbnailsOrder(ord,ui->actionDescending->isChecked());
}

void MViewer::on_actionList_all_triggered()
{
    QMessageBox::information(this,tr("Plugins list"),plugins.listPlugins());
}

void MViewer::on_actionReload_metadata_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    prepareLongProcessing();
    double prg = 0, dp = 100.f / (double)(ptm->GetAllImages().size());
    for (auto &i: ptm->GetAllImages()) {
        prg += dp;
        progressBar->setValue(floor(prg));
        QCoreApplication::processEvents();
        if (stop_flag) break;

        MImageExtras ex = db.getExtrasFromDB(i.filename);
        if (!ex.valid) continue; //we haven't scanned this file yet

        bool ok = false;
        qint64 sz;
        QByteArray sha = DBHelper::getSHA256(i.filename,&sz); //current, "real" file
        if (!sha.isEmpty() && sz == ex.filelen && sha == ex.sha) ok = true;

        if (ok) continue;
        ui->statusBar->showMessage("Updating "+i.fnshort);

        QPixmap pic(i.filename);
        ex = mCV.collectImageExtraData(i.filename,pic);
        ok = db.updateStatRecord(i.filename,ex);
        qDebug() << "File " << i.filename << " has changed. Updated: " << ok;

        if (ok) extra_cache.erase(i.filename);
    }

    checkExtraCache();
    prepareLongProcessing(true);
    ui->statusBar->showMessage(QString());
}

void MViewer::on_actionSanitize_DB_triggered()
{
    //step 0. remove unreachable files
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking files...");
    db.sanitizeFiles(prog_callback);
    prepareLongProcessing(true);

    //step 1. check all links
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking links...");
    db.sanitizeLinks(prog_callback);
    prepareLongProcessing(true);

    //step 2. renew tags ratings
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking tags...");
    db.sanitizeTags(prog_callback);

    //step 3. run various other DB engine-related stuff
    ui->statusBar->showMessage("DB maintenance...");
    db.sanitizeDBMeta();

    //step 4. reload cache
    db.invalidateCache();

    //process complete
    qDebug() << "[Sanitizer] Done.";
    prepareLongProcessing(true);
    ui->statusBar->showMessage("Done");
}

void MViewer::on_actionPrevious_triggered()
{
    if (!ui->listView->model()) return;
    if (ui->listView->currentIndex().row() < 1) return;

    MImageListModel* ptr = dynamic_cast<MImageListModel*>(ui->listView->model());
    if (!ptr) return;

    ui->listView->setCurrentIndex(ptr->getRecordIndex(ui->listView->currentIndex().row() - 1));
}

void MViewer::on_actionNext_triggered()
{
    if (!ui->listView->model()) return;

    MImageListModel* ptr = dynamic_cast<MImageListModel*>(ui->listView->model());
    if (!ptr) return;

    if (ui->listView->currentIndex().row() + 1 >= ptr->GetAllImages().size()) return;

    ui->listView->setCurrentIndex(ptr->getRecordIndex(ui->listView->currentIndex().row() + 1));
}

void MViewer::on_actionPrevious_2_triggered()
{
    if (history.files.empty() || history.cur == history.files.begin()) return;
    if (history.cur == history.files.end()) history.cur--;
    history.cur--;
    historyShowCurrent();
}

void MViewer::on_actionNext_2_triggered()
{
    if (history.files.empty() || history.cur == history.files.end()) return;
    history.cur++;
    if (history.cur == history.files.end()) return;
    historyShowCurrent();
}

void MViewer::historyShowCurrent()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm && !history.cur->isEmpty()) {
        //just select file in current list model
        QModelIndex idx = ptm->getRecordIndex(*history.cur);
        if (idx.isValid()) ui->listView->setCurrentIndex(idx);

    } else {
        //load file
        current_l = loader->loadFull(*history.cur);
        scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
        leftImageMetaUpdate();
    }
}

void MViewer::on_actionRandom_image_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm || ptm->GetAllImages().empty()) return;
    double idx = (double)random() / (double)RAND_MAX * (double)(ptm->GetAllImages().size());
    ui->listView->setCurrentIndex(ptm->getRecordIndex(floor(idx)));
}

void MViewer::on_actionClose_triggered()
{
    cleanUp();
}

void MViewer::on_actionFind_duplicates_triggered()
{
    prepareLongProcessing();

    ui->statusBar->showMessage("Checking for duplicates in DB...");
    QString report = db.detectExactCopies(prog_callback);

    bool abrt = stop_flag;
    prepareLongProcessing(true);
    ui->statusBar->showMessage(abrt? "Aborted":"Done");
    if (abrt || report.isEmpty()) return;

    QString fn = QFileDialog::getSaveFileName(this, tr("Save report to"), "", tr("Text Files (*.txt)"));
    if (fn.isEmpty()) return;
    if (fn.right(4).toUpper() != ".TXT") fn += ".txt";

    QFile fl(fn);
    if (!fl.open(QIODevice::Text | QIODevice::WriteOnly)) {
        qDebug() << "ALERT: unable to write " << fn;
        return;
    }
    QTextStream strm(&fl);
    strm << report;
    fl.close();
}

void MViewer::showGeneratedPicture(QPixmap const &in)
{
    if (in.isNull()) return; //skip if no input were given

    //get the previous frame offsets
    int pv = 0, ph = 0;
    if (current_r.generated && current_r.valid && !ui->actionFit->isChecked()) {
        QScrollBar* h = ui->scrollArea_2->horizontalScrollBar();
        if (h) ph = h->value();
        QScrollBar* v = ui->scrollArea_2->verticalScrollBar();
        if (v) pv = v->value();
    }

    //set current right image to generated one
    current_r = MImageListRecord();
    current_r.fnshort = "Generated Picture";
    current_r.picture = in;
    current_r.loaded = true;
    current_r.generated = true;
    current_r.valid = true;

    //and show it
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);

    //restore the previous frame offsets
    if (ph || pv) {
        QCoreApplication::processEvents();
        QScrollBar* h = ui->scrollArea_2->horizontalScrollBar();
        if (h) h->setValue(ph);
        QScrollBar* v = ui->scrollArea_2->verticalScrollBar();
        if (v) v->setValue(pv);
    }
}

void MViewer::enableShortcuts(QObjectList const &children, bool en)
{
    for (auto &i : children) {
        enableShortcuts(i->children(),en);
        if (QString(i->metaObject()->className()) != "QAction") continue;
        QAction* ptr = dynamic_cast<QAction*>(i);
        if (!ptr) continue;

        if (!en) {
            hotkeys[ptr] = ptr->shortcut();
            ptr->setShortcut(QKeySequence(""));

        } else if (hotkeys.count(ptr)) {
            ptr->setShortcut(hotkeys.at(ptr));
            hotkeys.erase(ptr);
        }
    }
}

void MViewer::on_actionRepeat_last_triggered()
{
    plugins.repeatLastPlugin();
}

void MViewer::on_actionAlways_show_GUI_toggled(bool arg1)
{
    plugins.setForceUI(arg1);
}

void MViewer::on_actionHotkeys_enabled_toggled(bool arg1)
{
    enableShortcuts(this->children(),arg1);
}

void MViewer::enableMouseMoveEvents(QObjectList const &lst)
{
    for (auto &i : lst) {
        enableMouseMoveEvents(i->children());
        QWidget* ptr = dynamic_cast<QWidget*>(i);
        if (ptr) ptr->setMouseTracking(true);
    }
}

void MViewer::on_actionClear_triggered()
{
    db.clearRecentDirs(true);
}

void MViewer::updateRecents()
{
    db.readRecentDirs(ui->menuRecent_dirs,MILLA_MAX_RECENT_DIRS,[this] (auto s) { this->showImageList(loader->open(s)); });
}

void MViewer::updateStory(QPixmap const &result)
{
    showGeneratedPicture(result);
    ui->label_6->setText(QString::asprintf("Step %d/%d",a_story->position()+1,a_story->size()));
    ui->plainTextEdit_2->setPlainText(a_story->getComment());
    ui->plainTextEdit_2->setEnabled(!result.isNull());
    QStringList l = a_story->getCurrentFileNames();
    ui->listWidget_2->clear();
    for (auto &i : l) ui->listWidget_2->addItem(i);
}

void MViewer::on_actionRotate_90_CW_triggered()
{
    if (current_l.valid) updateStory(a_story->rotate(current_l,true));
}

void MViewer::on_actionRotate_90_CCW_triggered()
{
    if (current_l.valid) updateStory(a_story->rotate(current_l,false));
}

void MViewer::on_actionFlip_vertical_triggered()
{
    if (current_l.valid) updateStory(a_story->flip(current_l,true));
}

void MViewer::on_actionFlip_horizontal_triggered()
{
    if (current_l.valid) updateStory(a_story->flip(current_l,false));
}

void MViewer::on_actionZoom_in_triggered()
{
    ui->actionCenter_on_face->setChecked(false);
    ui->actionShow_faces->setChecked(false);
    if (current_l.valid) scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,MILLA_SCALE_UP);
    if (current_r.valid) scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,MILLA_SCALE_UP);
}

void MViewer::on_actionZoom_out_triggered()
{
    ui->actionCenter_on_face->setChecked(false);
    ui->actionShow_faces->setChecked(false);
    if (current_l.valid) scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,MILLA_SCALE_DOWN);
    if (current_r.valid) scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,MILLA_SCALE_DOWN);
}

void MViewer::on_actionReset_zoom_triggered()
{
    scaleFactor = 1;
    if (current_l.valid) scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
    if (current_r.valid) scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
}

void MViewer::on_actionClear_all_triggered()
{
    MMemoryModel* mmm = dynamic_cast<MMemoryModel*>(ui->memList->model());
    if (mmm) mmm->clear(true);
}

void MViewer::on_lineEdit_2_textChanged(const QString &arg1)
{
    if (current_l.valid) updateTags(current_l.filename);
    else updateTags();

    for (int i = 0; i < ui->listWidget->count(); i++) {
        if (ui->listWidget->item(i)->text().contains(arg1)) continue;
        delete (ui->listWidget->takeItem(i));
        i--;
    }
}

void MViewer::copyTagsetTo(QString const &fn)
{
    if (fn.isEmpty()) return;

    MTagsCheckList lst = db.getFileTags(fn);
    MTagCache out;

    for (auto &j : lst) {
        if (!std::get<2>(j)) continue;
        out[std::get<0>(j)] = std::pair<unsigned,Qt::CheckState>(std::get<1>(j),Qt::Checked);
    }

    for (auto &j : tags_cache) {
        if (j.second.second != Qt::Checked) continue;
        if (!out.count(j.first)) db.updateTags(j.first,true); //increment tag counter
        out[j.first] = j.second;
    }

    bool ok = db.updateFileTags(fn,out);
    qDebug() << "[Tags] Applying to" << fn << ":" << ok;
}

void MViewer::on_actionApply_tagset_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!current_l.valid || current_l.generated || !ptm) return;

    double prg = 0, dp = 100.f / (double)(ptm->GetAllImages().size());

    prepareLongProcessing();
    for (auto &i : ptm->GetAllImages()) {
        copyTagsetTo(i.filename);
        prg += dp;
        progressBar->setValue(floor(prg));
        QCoreApplication::processEvents();
        if (stop_flag) break;
    }
    prepareLongProcessing(true);
    ui->statusBar->showMessage("Finished setting tags");
}

void MViewer::on_actionApply_tagset_from_left_to_right_triggered()
{
    if (!current_l.valid || !current_r.valid || current_r.generated) return;
    if (!db.isStatRecordExists(current_r.filename)) return;

    copyTagsetTo(current_r.filename);
    ui->statusBar->showMessage("Tags copied");
}

void MViewer::on_actionConcatenate_triggered()
{
    if (current_l.valid) updateStory(a_story->concatenate(current_l,current_r));
}

void MViewer::on_pushButton_9_clicked()
{
    //TODO: remove single step (with confirmation box)
}

void MViewer::on_pushButton_4_clicked()
{
    if (!a_story->isActive()) return;
    a_story->addComment(ui->plainTextEdit_2->toPlainText());
    updateStory(a_story->previous(ui->actionSkip_empty_story_steps->isChecked()));
}

void MViewer::on_pushButton_6_clicked()
{
    if (!a_story->isActive()) return;
    a_story->addComment(ui->plainTextEdit_2->toPlainText());
    updateStory(a_story->next(ui->actionSkip_empty_story_steps->isChecked()));
}

void MViewer::on_pushButton_8_clicked()
{
    if (!a_story->isActive()) return;
    a_story->addComment(ui->plainTextEdit_2->toPlainText());
    if (db.updateStory(ui->lineEdit_3->text().replace('\"','\''),a_story)) {
        ui->statusBar->showMessage("Story saved");
        a_story->clearDirty();
    }
}

void MViewer::on_pushButton_5_clicked()
{
    if (!a_story->isActive()) return;
    a_story->addComment(ui->plainTextEdit_2->toPlainText());
    if (a_story->moveCurrent(true)) updateStory(a_story->current());
}

void MViewer::on_pushButton_7_clicked()
{
    if (!a_story->isActive()) return;
    a_story->addComment(ui->plainTextEdit_2->toPlainText());
    if (a_story->moveCurrent(false)) updateStory(a_story->current());
}

void MViewer::on_actionPick_a_story_triggered()
{
    if (a_story->isDirty()) {
        if (QMessageBox::question(this, tr("Question"), tr("Do you want to save current story?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            on_pushButton_8_clicked();
    }

    StorySelector dlg;
    if (!dlg.exec()) return;

    if (db.loadStory(dlg.getStoryTitle(),a_story)) {
        updateStory(a_story->first());
        ui->lineEdit_3->setText(dlg.getStoryTitle());
        ui->tabWidget->setCurrentIndex(MVTAB_STORY);
        ui->statusBar->showMessage("Story loaded.");
    } else
        ui->statusBar->showMessage("Unable to load story selected!");
}

void MViewer::on_actionAdd_to_story_triggered()
{
    if (current_l.valid) updateStory(a_story->append(current_l));
}

void MViewer::on_actionCrop_triggered()
{
    if (selection_fsm != 2 || !current_l.valid) return;
    updateStory(a_story->crop(current_l,(selection_scaled & current_l.picture.rect())));
}

bool MViewer::eventFilter(QObject *obj, QEvent *event)
{
    QMouseEvent* mev;
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
        mev = static_cast<QMouseEvent*>(event);
        break;

    default:
        return QObject::eventFilter(obj,event);
    }

    //check event source - is it from one of top labels?
    if (obj == ui->label_3 || obj == ui->label_4) {
        if (event->type() != QEvent::MouseButtonPress) return QObject::eventFilter(obj,event);
        if (obj == ui->label_3 && current_l.valid)
            QGuiApplication::clipboard()->setText(current_l.filename);
        else if (obj == ui->label_4 && current_r.valid)
            QGuiApplication::clipboard()->setText(current_r.filename);
        return true;
    }

    //evrything down here is related to left ScrollArea
    QRect aligned = QStyle::alignedRect(QApplication::layoutDirection(),QFlag(ui->label->alignment()),ui->label->pixmap()->size(),ui->label->rect());
    QRect inter = aligned.intersected(ui->label->rect());
    QPoint scrl_delta(0,0);
    if (ui->scrollArea->horizontalScrollBar()) scrl_delta.setX(ui->scrollArea->horizontalScrollBar()->value());
    if (ui->scrollArea->verticalScrollBar()) scrl_delta.setY(ui->scrollArea->verticalScrollBar()->value());

    switch (selection_fsm) {
    case 0:
        selection_bak = *(ui->label->pixmap());
        selection_fsm = 10;
        //fall through
    case 10:
    case 2:
        if (event->type() == QEvent::MouseButtonPress) {
            ui->label->setPixmap(selection_bak);
            selection.setTopLeft(QPoint(mev->x()-inter.x(),mev->y()-inter.y())+scrl_delta);
            selection_fsm = 1;
        }
        break;
    case 1:
        if (event->type() == QEvent::MouseMove) {
            selection.setBottomRight(QPoint(mev->x()-inter.x(),mev->y()-inter.y())+scrl_delta);

        } else if (event->type() == QEvent::MouseButtonRelease) {
            selection.setBottomRight(QPoint(mev->x()-inter.x(),mev->y()-inter.y())+scrl_delta);
            qDebug() << "Selection size = " << selection.width() << " x " << selection.height();
            if (selection.width() < 2 && selection.height() < 2) {
                selection_fsm = 10;
                ui->label->setPixmap(selection_bak);

            } else {
                float scale = (current_l.picture.size().width() > current_l.picture.size().height())?
                            (double(current_l.picture.size().width()) / double(ui->scrollArea->widget()->rect().width())) :
                            (double(current_l.picture.size().height()) / double(ui->scrollArea->widget()->rect().height()));
                selection_scaled = QRect(selection.topLeft()*scale,selection.size()*scale);

                selection_fsm++;
            }
        }
        break;
    default:
        selection_fsm = 10;
        break;
    }

    if (current_l.valid && selection_fsm && selection_fsm < 10) {
        QImage tmp = selection_bak.toImage();
        QPainter painter(&tmp);
        QPen paintpen(Qt::black);
        paintpen.setWidth(1);
        paintpen.setStyle(Qt::DashLine);
        painter.setPen(paintpen);
        painter.drawRect(selection);
        ui->label->setPixmap(QPixmap::fromImage(tmp));
    }

    return true;
}

QString MViewer::printInfo(QString const &title, MImageListRecord const &targ)
{
    if (!targ.valid || targ.generated) return QString();

    QString res(title);
    res += " image:\n";
    res += targ.fnshort + '\n';
    res += targ.filename + '\n';
    return res;
}

void MViewer::on_actionInfo_triggered()
{
    QString inf = printInfo("Left",current_l);
    if (current_l.valid && current_r.valid) inf += '\n';
    inf += printInfo("Right",current_r);
    if (!inf.isEmpty()) QMessageBox::information(this,tr("Info"),inf);
}

void MViewer::on_actionOpen_with_triggered()
{
    if (!current_l.valid) return;

    bool ok;
    QString orig = DBHelper::getExtraStringVal(DBF_EXTRA_EXTERNAL_EDITOR);
    QStringList lst = orig.split('\n',Qt::SkipEmptyParts);
    int def = DBHelper::getExtraInt(DBF_EXTRA_EXTERNAL_EDITOR);
    QString cmd = QInputDialog::getItem(this,tr("Open with external program"),tr("Select or enter command"),lst,def,true,&ok);

    if (ok && !cmd.isEmpty()) {
        if (lst.contains(cmd)) {
            // we know this command, update only the default choice
            def = lst.indexOf(cmd);
        } else {
            // append new command to the list of known commands
            DBHelper::setExtraStringVal(DBF_EXTRA_EXTERNAL_EDITOR,orig+"\n"+cmd);
            def = lst.size();
        }
        DBHelper::setExtraInt(DBF_EXTRA_EXTERNAL_EDITOR,def);

        cmd += " \"";
        if (current_l.generated) {
            QString fn = "/tmp/milla_temp_image.png"; // FIXME: make it properly!
            ok = current_l.picture.save(fn);
            qDebug() << "Saving generated left image to " << fn << ": " << ok;
            if (!ok) return;
            cmd += fn;
        } else if (!current_l.filename.isEmpty())
            cmd += current_l.filename;
        else {
            qDebug() << "ERROR: not a generated image, but also have an empty filename, nothing to do";
            return;
        }
        cmd += "\" &";
        qDebug() << "[RUN] " << cmd;

        //TODO: portable and safer version
        system(cmd.toStdString().c_str()); //WARNING: potentially insecure, and Linux-only
    }
}

void MViewer::on_actionEdit_exclusion_list_triggered()
{
    ListEditor dlg;
    dlg.setTextLabel(tr("Add or remove text fragments, which will exclude particular paths from search list once they're found in that paths."));
    dlg.setList(DBHelper::getExtraStringVal(DBF_EXTRA_EXCLUSION_LIST).split(';',QString::SkipEmptyParts));

    if (dlg.exec())
        DBHelper::setExtraStringVal(DBF_EXTRA_EXCLUSION_LIST,dlg.getList().join(';'));
}

void MViewer::on_actionSave_right_image_as_triggered()
{
    if (!current_r.valid || current_r.picture.isNull()) return;

    QString flt(MILLA_SAVE_FILE);
    QString fn = QFileDialog::getSaveFileName(this,tr("Save as"),"",flt);
    if (fn.isEmpty()) return;

    bool ok = current_r.picture.save(fn);
    qDebug() << "Saving right image to " << fn << ": " << ok;
}

void MViewer::linkageAction(bool link)
{
    if (!current_l.valid || !current_r.valid || current_l.generated || current_r.generated) return;

    MImageExtras extl = getExtraCacheLine(current_l.filename);
    MImageExtras extr = getExtraCacheLine(current_r.filename);
    checkExtraCache();
    if (!extl.valid || !extr.valid || extl.sha.isEmpty() || extr.sha.isEmpty()) return;

    if (link) {
        if (db.createLinkBetweenImages(extl.sha,extr.sha)) ui->statusBar->showMessage("Linked");
        else ui->statusBar->showMessage("Unable to link images");
    } else {
        if (db.removeLinkBetweenImages(extl.sha,extr.sha)) ui->statusBar->showMessage("Unlinked");
        else ui->statusBar->showMessage("Unable to unlink images");
    }

    if (ui->actionShow_linked_image->isChecked()) displayLinkedImages(current_l.filename);
}

void MViewer::on_actionUnlink_images_triggered()
{
    linkageAction(false);
}

void MViewer::on_actionJump_next_triggered()
{
    if (jump_buf.isEmpty()) return;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm)
        ui->listView->setCurrentIndex(ptm->getRecordIndex(jump_buf,true,&jump_idx));
}

void MViewer::on_actionLoad_right_image_dir_triggered()
{
    if (!current_r.valid || current_r.generated || current_r.filename.isEmpty()) return;
    showImageList(loader->open(current_r.filename));
}

void MViewer::showFaceAction()
{
    if (!current_l.valid || current_l.generated) return;

    MImageExtras extl = getExtraCacheLine(current_l.filename);
    if (!extl.valid || extl.rois.empty()) return;

    int mx = 0, tx = 0;
    for (auto &i : extl.rois) {
        if (i.kind == MROI_FACE_FRONTAL) {
            if (mx == face_idx) face_idx_roi = tx;
            mx++;
        }
        tx++;
    }
    if (face_idx >= mx) face_idx = mx - 1;
    if (face_idx < 0) face_idx = 0;
    qDebug() << "Showing face #" << face_idx;

    ui->actionFit->setChecked(false);
    ui->actionShow_faces->setChecked(true);
    ui->actionCenter_on_face->setChecked(true);
    scaleFactor = 1.f;
    scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1.f);
}

void MViewer::on_actionNext_face_triggered()
{
    face_idx++;
    showFaceAction();
}

void MViewer::on_actionPrevious_face_triggered()
{
    face_idx--;
    showFaceAction();
}

void MViewer::on_actionExport_found_triggered()
{
    SResultModel* ptm = dynamic_cast<SResultModel*>(ui->listView_2->model());
    if (!ptm) return;

    QString fn = QFileDialog::getSaveFileName(this,tr("Save as"),"","Text files (*.txt, *.lst)");
    if (fn.isEmpty()) return;

    if (fn.right(4).toUpper() != ".LST" && fn.right(4).toUpper() != ".TXT")
        fn += ".lst"; //auto-complete file extension

    QFile olst(fn);
    if (!olst.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "ALERT: unable to write file " << fn;
        return;
    }

    QString s;
    for (auto &i : ptm->GetAllImages()) {
        s = i.filename + '\n';
        olst.write(s.toStdString().c_str());
    }
    olst.close();
    ui->statusBar->showMessage("Finished");
}

void MViewer::on_actionRestore_startup_layout_triggered()
{
    updateWindowLayout("window");
    ui->statusBar->showMessage("Session startup layout restored");
}

void MViewer::on_actionLayout_1_triggered()
{
    updateWindowLayout("layout1");
    ui->statusBar->showMessage("User layout 1 restored");
}

void MViewer::on_actionLayout_2_triggered()
{
    updateWindowLayout("layout2");
    ui->statusBar->showMessage("User layout 2 restored");
}

void MViewer::on_actionLayout_3_triggered()
{
    updateWindowLayout("layout3");
    ui->statusBar->showMessage("User layout 3 restored");
}

void MViewer::on_actionLayout_4_triggered()
{
    updateWindowLayout("layout1",true);
    ui->statusBar->showMessage("User layout 1 saved");
}

void MViewer::on_actionLayout_5_triggered()
{
    updateWindowLayout("layout2",true);
    ui->statusBar->showMessage("User layout 2 saved");
}

void MViewer::on_actionLayout_6_triggered()
{
    updateWindowLayout("layout3",true);
    ui->statusBar->showMessage("User layout 3 saved");
}

void MViewer::on_actionFill_rect_triggered()
{
    if (selection_fsm != 2 || !current_l.valid) return;
    updateStory(a_story->fillrect(current_l,(selection_scaled & current_l.picture.rect())));
}

void MViewer::on_actionDesaturate_triggered()
{
    if (current_l.valid) updateStory(a_story->desaturate(current_l));
}

void MViewer::on_actionColorize_triggered()
{
    if (current_l.valid) updateStory(a_story->colorize(current_l));
}

void MViewer::on_actionContinue_search_triggered()
{
    prepareLongProcessing();
    QStringList res = db.doParametricSearch(last_search,prog_callback);
    prepareLongProcessing(true);
    if (res.empty())
        QMessageBox::information(this,tr("Search"),tr("Nothing found. Try to relax the search parameters."));
    else
        searchResults(res);
    ui->actionContinue_search->setEnabled(!res.empty());
}

void MViewer::on_listWidget_2_itemDoubleClicked(QListWidgetItem *item)
{
    if (!db.isStatRecordExists(item->text())) return;
    current_r = loader->loadFull(item->text());
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
    incViews(false);
}

void MViewer::on_actionClear_story_triggered()
{
    if (QMessageBox::question(this, tr("Confirmation"), tr("Do you want to erase all steps in the current story?"), QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    a_story->clear();
    ui->label_6->setText("Step 0/0");
    ui->lineEdit_3->clear();
    ui->plainTextEdit_2->clear();
    ui->listWidget_2->clear();
}

void MViewer::on_actionEqualize_triggered()
{
    if (current_l.valid) updateStory(a_story->equalize(current_l));
}

void MViewer::on_actionPlay_special_file_triggered()
{
    if (current_l.valid && !current_l.filename.isEmpty()) plugins.openFileFormat(current_l.filename);
}

void MViewer::on_actionAdd_directory_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) return;

    MDirectoryModel* mdm = dynamic_cast<MDirectoryModel*>(ui->dirList->model());
    if (mdm) mdm->addDir(dir);
}

void MViewer::on_actionRemove_directory_triggered()
{
    MDirectoryModel* mdm = dynamic_cast<MDirectoryModel*>(ui->dirList->model());
    if (mdm) mdm->delDir(ui->dirList->selectionModel()->currentIndex().data(Qt::DisplayRole).toString());
}

void MViewer::on_actionClear_dirs_triggered()
{
    MDirectoryModel* mdm = dynamic_cast<MDirectoryModel*>(ui->dirList->model());
    if (mdm && QMessageBox::question(this, tr("Confirmation"), tr("Do you want to remove all directories from the list?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        mdm->delAll();
}

void MViewer::attemptCopyMoveToDir(bool move)
{
    QString dst = ui->dirList->selectionModel()->currentIndex().data(Qt::DisplayRole).toString();
    QFile src(current_l.filename);
    if (dst.isEmpty() || !src.exists() || !current_l.valid) return;

    QFileInfo inf(current_l.filename);
    QString fn = dst + "/" + inf.fileName();
    bool ok = src.copy(fn);
    qDebug() << "Copying " << current_l.filename << " to " << fn << ": " << ok << endl;
    if (!ok) return;

    MDirectoryModel* mdm = dynamic_cast<MDirectoryModel*>(ui->dirList->model());
    if (mdm) mdm->updateDir(dst,fn);

    if (!move) return;

    if (ui->actionMove_files_to_trash_instead_deleting->isChecked()) {
        ok = src.moveToTrash();
        qDebug() << "Moving " << current_l.filename << " to trash: " << ok << endl;
    } else {
        ok = src.remove();
        qDebug() << "Deleting " << current_l.filename << ": " << ok << endl;
    }
    if (!ok) return;

    view_timer.stop();
    current_l.valid = false;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->deleteRecordByFullName(current_l.filename);
    ui->listView->update();
}

void MViewer::on_actionCopy_to_dir_triggered()
{
    attemptCopyMoveToDir(false);
}

void MViewer::on_actionMove_to_dir_triggered()
{
    attemptCopyMoveToDir(true);
}

void MViewer::on_actionOpen_dir_triggered()
{
    MDirectoryModel* mdm = dynamic_cast<MDirectoryModel*>(ui->dirList->model());
    if (!mdm) return;

    QString dir = ui->dirList->selectionModel()->currentIndex().data(Qt::DisplayRole).toString();
    if (dir.isEmpty()) return;
    showImageList(loader->open(dir));
}
