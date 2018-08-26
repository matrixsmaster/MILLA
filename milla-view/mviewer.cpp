#include "mviewer.h"
#include "ui_mviewer.h"
#include "searchform.h"

MViewer::MViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MViewer)
{
    ui->setupUi(this);

    if (!db.initDatabase()) {
        QMessageBox::critical(this, tr("Fatal error"), tr("Unable to initialize database"));
        qDebug() << "FATAL: Unable to initialize database " DB_FILEPATH;

        connect(&view_timer,&QTimer::timeout,this,[this] { QApplication::exit(1); });
        view_timer.start(2);
        return;
    }

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

    thumbload_pbar = ([this] (auto v) {
        this->progressBar->setValue(floor(v));
        QCoreApplication::processEvents();
        return true;
    });

    connect(&view_timer,&QTimer::timeout,this,[this] {
        if (progressBar->value() < 100)
            progressBar->setValue(progressBar->value()+1);
        else {
            view_timer.stop();
            ui->lcdNumber->display((double)incViews());
        }
    });
    connect(stopButton,&QPushButton::clicked,this,[this] { stop_flag = true; });

    connect(ui->star_1,&StarLabel::clicked,this,[this] { changedStars(1); });
    connect(ui->star_2,&StarLabel::clicked,this,[this] { changedStars(2); });
    connect(ui->star_3,&StarLabel::clicked,this,[this] { changedStars(3); });
    connect(ui->star_4,&StarLabel::clicked,this,[this] { changedStars(4); });
    connect(ui->star_5,&StarLabel::clicked,this,[this] { changedStars(5); });

    enableMouseMoveEvents(children());

    plugins.setViewerContext(MillaPluginContext({ &current_l,ui->scrollArea_2,ui->label_2,this }));
    plugins.addPluginsToMenu(*(ui->menuPlugins), [this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

    MMemoryModel* mmm = new MMemoryModel(loader,ui->listView_4);
    ui->listView_4->setModel(mmm);
    ui->listView_4->setViewMode(QListView::ListMode);
    ui->listView_4->setFlow(QListView::LeftToRight);
    ui->listView_4->setWrapping(false);
    ui->listView_4->setWordWrap(true);
    for (int i = 0; i < MAXMEMORYSLOTS; i++) {
        QAction* a = ui->menuShort_term_memory->addAction(QString::asprintf("Slot %d",i+1));
        if (!a) break;
        a->setShortcut(QKeySequence(QString::asprintf("F%d",i+1)));
        connect(a,&QAction::triggered,this,[i,mmm,this] {
            if (this->current_l.valid && !this->current_l.generated) {
                mmm->setSlot(i,this->current_l);
                ui->tabWidget->setCurrentIndex(3);
            }
        });
    }
    connect(ui->listView_4->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->listView_4->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });

    ui->scrollArea->installEventFilter(this);

    cleanUp();
    updateTags();
    updateRecents();

    restoreGeometry(db.getWindowGeometryOrState(true));
    restoreState(db.getWindowGeometryOrState(false));
    db.restoreViewerState(children());
}

MViewer::~MViewer()
{
    db.updateWindowGeometryAndState(saveGeometry(),saveState());
    db.updateViewerState(children());
    delete ui;
}

void MViewer::cleanUp()
{
    extra_cache.clear();

    current_l = MImageListRecord();
    current_r = MImageListRecord();

    history.files.clear();
    history.cur = history.files.begin();

    if (ui->listView->model()) ui->listView->model()->deleteLater();
    if (ui->listView_2->model()) ui->listView_2->model()->deleteLater();
    if (ui->listView_3->model()) ui->listView_3->model()->deleteLater();

    ui->label->setPixmap(QPixmap());
    ui->label_2->setPixmap(QPixmap());
    ui->label_3->clear();
    ui->label_4->clear();

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

void MViewer::showImageList(QStringList const &lst)
{
    cleanUp();

    bool purelist = !(ui->actionThumbnails_cloud->isChecked());
    ThumbnailModel* ptr = new ThumbnailModel(lst,thumbload_pbar,loader,ui->listView);

    ptr->setShortenFilenames(!purelist);
    ui->listView->setModel(ptr);
    ui->listView->setViewMode(purelist? QListView::ListMode : QListView::IconMode);
    ui->listView->setFlow(purelist? QListView::TopToBottom : QListView::LeftToRight);
    ui->listView->setWrapping(!purelist);
    ui->listView->setSpacing(purelist? 5:10);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView->selectionModel(),&QItemSelectionModel::selectionChanged,[this] { showSelectedImage(); });
    connect(ui->listView,&QListView::customContextMenuRequested,this,[this] {
        current_r = ui->listView->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });

    ui->statusBar->showMessage(QString::asprintf("%d images",lst.size()));
    updateRecents();
    on_actionDescending_triggered();
    on_pushButton_9_clicked();
}

void MViewer::showSelectedImage()
{
    if (ui->listView->selectionModel()->selectedIndexes().isEmpty()) return;

    QModelIndex idx = ui->listView->selectionModel()->selectedIndexes().first();
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->LoadUp(idx.row());
    else return;
    ptm->touch(idx);

    current_l = idx.data(MImageListModel::FullDataRole).value<MImageListRecord>();
    scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
    leftImageMetaUpdate();
    checkExtraCache();
    ui->lineEdit_2->clear();

    if (ui->actionAutoplay_special_files->isChecked())
        qDebug() << "Opening file as a special file format: " << plugins.openFileFormat(current_l.filename);

    view_timer.start(MILLA_VIEW_TIMER);
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
                                      rec.fnshort.toStdString().c_str(),
                                      rec.picture.size().width(),
                                      rec.picture.size().height()));

    if (ui->actionFit->isChecked())
        lbl->setPixmap(rec.picture.scaled(lbl->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));

    else {
        QSize nsz = rec.picture.size() * scaleFactor;
        MImageExtras extr = getExtraCacheLine(rec.filename);

        if (ui->actionShow_faces->isChecked() || ui->actionCenter_on_face->isChecked()) {
            QRect vbd;
            lbl->setPixmap(CVHelper::drawROIs(rec.picture,vbd,extr,!ui->actionShow_faces->isChecked()));

            if (ui->actionCenter_on_face->isChecked()) {
                //it takes some time to actually show the image, so wait until next GUI cycle
                QCoreApplication::processEvents();
                //now center on face
                scrl->ensureVisible(vbd.x(),vbd.y(),vbd.width(),vbd.height());
            }

        } else
            lbl->setPixmap(rec.picture.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }

    selection_fsm = 0; //invalidate selection
}

unsigned MViewer::incViews(bool left)
{
    if ((left && !current_l.valid) || (!left && !current_r.valid)) return 0;
    QString fn = (left? current_l : current_r).filename;
    if (fn.isEmpty()) return 0;
    qDebug() << "Incrementing views counter for " << fn;

    bool ok;
    unsigned v = db.getFileViews(fn,ok);
    if (!ok) {
        //first time watching, let's create a record
        if (!createStatRecord(fn)) return 0;
    }
    v++;
    if (!db.updateFileViews(fn,v)) return 0;

    if (left) {
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
    }
    kudos(current_l,0);

    ui->radio_settags->setChecked(true);
    progressBar->setValue(0);
    ui->statusBar->showMessage("");
}

void MViewer::processArguments()
{
    QStringList args = QApplication::arguments();
    if (args.size() < 2) {
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
    if (!orig.valid) return;

    //destroy model immediately, to prevent it from loading more images (if background loading is activated)
    SResultModel* mdl = static_cast<SResultModel*>(ui->listView_2->model());
    ui->listView_2->setModel(nullptr);
    delete mdl;

    MMatcher match(orig,current_l.filename,MILLA_MAXMATCH_RESULTS);
    QStringList lst;

    if (ui->actionGlobal_search->isChecked()) {
        prepareLongProcessing();
        lst = match.GlobalMatcher([this] (double p) {
            progressBar->setValue(floor(p));
            QCoreApplication::processEvents();
            return !stop_flag;
        });
        prepareLongProcessing(true);

    } else {
        ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
        if (!ptm) return;
        lst = match.LocalMatcher(ptm->GetAllImages(),[this] (auto s) { return this->getExtraCacheLine(s); });
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
        if (!createStatRecord(i.filename)) continue;

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
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    bool fnd = false;

    if (ptm) {
        QList<MImageListRecord> &imgs = ptm->GetAllImages();
        for (auto &i : lst) {
            int idx = 0;

            for (auto &j : imgs) {
                if (j.filename == i) break;
                idx++;
            }
            if (idx >= imgs.size()) break;

            ptm->LoadUp(idx);
            out.push_back(imgs.at(idx));
            fnd = true;
        }
    }

    if (!fnd) {
        for (auto &i : lst) {
            MImageListRecord r;
            r.filename = i;
            out.push_back(r);
        }
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
    resultsPresentation(lst,ui->listView_2,1);

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

    if (ok) ui->listView->setCurrentIndex(ptm->getRecordIndex(fn,true));
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
            if (ex.valid) flt.similar_to = ex.hist;
            else flt.similar = 0;
        }
    } else
        flt.similar = 0;

    last_search = flt;
    search_cnt++;

    bool glob = false;
    MImageListModel* ptm = NULL;
    switch (flt.scope) {
    case SRSCP_NEW:
        ptm = dynamic_cast<MImageListModel*>(ui->listView->model());
        glob = ui->actionGlobal_search->isChecked();
        search_exclusions.clear();
        break;
    case SRSCP_FOUND:
        ptm = dynamic_cast<MImageListModel*>(ui->listView_2->model());
        break;
    case SRSCP_LINKS:
        ptm = dynamic_cast<MImageListModel*>(ui->listView_3->model());
        break;
    case SRSCP_CONTINUE:
        ptm = dynamic_cast<MImageListModel*>(ui->listView_2->model());
        if (ptm) {
            for (auto &i : ptm->GetAllImages())
                search_exclusions.insert(i.filename);
        }
        ptm = dynamic_cast<MImageListModel*>(ui->listView->model());
        glob = ui->actionGlobal_search->isChecked();
        qDebug() << search_exclusions;
        break;
    }

    QStringList res;
    ProgressCB pcb = ([this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

    prepareLongProcessing();
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
        res = db.parametricSearch(flt,lst,search_exclusions,pcb);
    } else
        res = db.parametricSearch(flt,ptm->GetAllImages(),search_exclusions,pcb);
    prepareLongProcessing(true);

    if (res.empty()) QMessageBox::information(this,tr("Search"),tr("Nothing found. Try to relax the search parameters."));
    else searchResults(res);
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
    search_exclusions.clear();
    search_cnt = 0;
}

void MViewer::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MViewer::on_actionLink_left_to_right_triggered()
{
    if (!current_l.valid || !current_r.valid || current_l.generated || current_r.generated) return;

    MImageExtras extl = getExtraCacheLine(current_l.filename);
    MImageExtras extr = getExtraCacheLine(current_r.filename);
    if (!extl.valid || !extr.valid || extl.sha.isEmpty() || extr.sha.isEmpty()) return;

    if (db.createLinkBetweenImages(extl.sha,extr.sha)) ui->statusBar->showMessage("Linked");
    else ui->statusBar->showMessage("Unable to link images");
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
    MImageExtras extr = getExtraCacheLine(fn);
    if (!extr.valid) return;

    QStringList out = db.getLinkedImages(extr.sha,false);
    if (ui->actionShow_reverse_links->isChecked())
        out += db.getLinkedImages(extr.sha,true);

    resultsPresentation(out,ui->listView_3,2);
    connect(ui->listView_3->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->listView_3->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
        incViews(false);
    });
}

void MViewer::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About MILLA"),
                       tr("<p><b>MILLA</b> image viewer</p>"
                          "<p><i>" MILLA_VERSION "</i></p>"
                          "<p><a href=" MILLA_SITE ">GitHub site</a></p>"
                          "<p>(C) Dmitry 'MatrixS_Master' Soloviov, 2018</p>"));
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
    if (!ptm) return;

    QStringList lst;
    for (auto &i : ptm->GetAllImages()) lst.push_back(i.filename);
    showImageList(lst);
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
    mod.setProgressBar([this] (double v) {
        this->progressBar->setValue(floor(v));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

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
    ProgressCB cb = ([this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

    //step 0. remove unreachable files
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking files...");
    db.sanitizeFiles(cb);
    prepareLongProcessing(true);

    //step 1. check all links
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking links...");
    db.sanitizeLinks(cb);
    prepareLongProcessing(true);

    //step 2. renew tags ratings
    prepareLongProcessing();
    ui->statusBar->showMessage("Checking tags...");
    db.sanitizeTags(cb);

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
    QString report = db.detectExactCopies([this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !stop_flag;
    });

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

    //set current right image to generated one
    current_r = MImageListRecord();
    current_r.fnshort = "Generated Picture";
    current_r.picture = in;
    current_r.loaded = true;
    current_r.generated = true;
    current_r.valid = true;

    //and show it
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
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
    ui->label_6->setText(QString::asprintf("Step %d/%d",a_story->position(),a_story->size()));
    ui->plainTextEdit_2->setPlainText(a_story->getComment());
    ui->plainTextEdit_2->setEnabled(!result.isNull());
}

void MViewer::on_actionRotate_90_CW_triggered()
{
    updateStory(a_story->rotate(current_l,true));
}

void MViewer::on_actionRotate_90_CCW_triggered()
{
    updateStory(a_story->rotate(current_l,false));
}

void MViewer::on_actionFlip_vertical_triggered()
{
    updateStory(a_story->flip(current_l,true));
}

void MViewer::on_actionFlip_horizontal_triggered()
{
    updateStory(a_story->flip(current_l,false));
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
    MMemoryModel* mmm = dynamic_cast<MMemoryModel*>(ui->listView_4->model());
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
    updateStory(a_story->concatenate(current_l,current_r));
}

void MViewer::on_pushButton_9_clicked()
{
    a_story->clear();
    ui->label_6->setText("Step 0/0");
    ui->lineEdit_3->clear();
    ui->plainTextEdit_2->clear();
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
    if (db.updateStory(ui->lineEdit_3->text().replace('\"','\''),a_story))
        ui->statusBar->showMessage("Story saved");
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
    if (!ui->lineEdit_3->text().isEmpty()) {
        if (QMessageBox::question(this, tr("Question"), tr("Do you want to save current story?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            on_pushButton_8_clicked();
    }

    StorySelector dlg;
    if (!dlg.exec() || dlg.getStoryTitle().isEmpty()) return;

    if (db.loadStory(dlg.getStoryTitle(),a_story)) {
        updateStory(a_story->first());
        ui->lineEdit_3->setText(dlg.getStoryTitle());
        ui->tabWidget->setCurrentIndex(4);
        ui->statusBar->showMessage("Story loaded.");
    } else
        ui->statusBar->showMessage("Unable to load story selected!");
}

void MViewer::on_actionAdd_to_story_triggered()
{
    updateStory(a_story->append(current_l));
}

void MViewer::on_actionCrop_triggered()
{
    if (selection_fsm != 2 || !current_l.valid) return;
    float scale = (current_l.picture.size().width() > current_l.picture.size().height())?
                (double(current_l.picture.size().width()) / double(ui->scrollArea->size().width())) :
                (double(current_l.picture.size().height()) / double(ui->scrollArea->size().height()));
    QRect ns(selection.topLeft()*scale,selection.size()*scale);
    updateStory(a_story->crop(current_l,ns & current_l.picture.rect()));
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

    QRect aligned = QStyle::alignedRect(QApplication::layoutDirection(),QFlag(ui->label->alignment()),ui->label->pixmap()->size(),ui->label->rect());
    QRect inter = aligned.intersected(ui->label->rect());

    switch (selection_fsm) {
    case 0:
        selection_bak = *(ui->label->pixmap());
        //fall through
    case 2:
        if (event->type() == QEvent::MouseButtonPress) {
            selection.setTopLeft(QPoint(mev->x()-inter.x(),mev->y()-inter.y()));
            selection_fsm = 1;
        }
        break;
    case 1:
        if (event->type() == QEvent::MouseMove) {
            selection.setBottomRight(QPoint(mev->x()-inter.x(),mev->y()-inter.y()));

        } else if (event->type() == QEvent::MouseButtonRelease) {
            selection.setBottomRight(QPoint(mev->x()-inter.x(),mev->y()-inter.y()));
            selection_fsm++;
        }
        break;
    default:
        selection_fsm = 0;
        break;
    }

    if (current_l.valid && selection_fsm) {
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

void MViewer::printInfo(QString title, MImageListRecord const &targ)
{
    title += " image:\n";
    title += targ.fnshort + '\n';
    title += targ.filename;

    QMessageBox::information(this,tr("Info"),title);
}

void MViewer::on_actionInfo_triggered()
{
    if (current_l.valid) printInfo("Left",current_l);
    else if (current_r.valid) printInfo("Right",current_r);
}

void MViewer::on_actionOpen_with_triggered()
{
    if (!current_l.valid) return;

    bool ok;
    QString cmd = DBHelper::getExtraStringVal("external_editor");
    cmd = QInputDialog::getText(this,tr("Open with external program"),tr("Enter command"),QLineEdit::Normal,cmd,&ok);

    if (ok && !cmd.isEmpty()) {
        DBHelper::setExtraStringVal("external_editor",cmd);
        cmd += ' ' + current_l.filename;
        cmd += " &";
        qDebug() << "[RUN] " << cmd;
        //TODO: portable and safer version
        system(cmd.toStdString().c_str()); //WARNING: potentially insecure, and Linux-only
    }
}
