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

    progressBar = new QProgressBar(this);
    progressBar->setTextVisible(false);
    ui->statusBar->addPermanentWidget(progressBar);

    stopButton = new QPushButton(this);
    stopButton->setText("stop");
    stopButton->setEnabled(false);
    ui->statusBar->addPermanentWidget(stopButton);

    connect(&view_timer,&QTimer::timeout,this,[this] {
        if (progressBar->value() < 100)
            progressBar->setValue(progressBar->value()+1);
        else {
            view_timer.stop();
            ui->lcdNumber->display((double)incViews());
        }
    });
    connect(stopButton,&QPushButton::clicked,this,[this] { flag_stop_load_everything = true; });

    connect(ui->star_1,&StarLabel::clicked,this,[this] { changedStars(1); });
    connect(ui->star_2,&StarLabel::clicked,this,[this] { changedStars(2); });
    connect(ui->star_3,&StarLabel::clicked,this,[this] { changedStars(3); });
    connect(ui->star_4,&StarLabel::clicked,this,[this] { changedStars(4); });
    connect(ui->star_5,&StarLabel::clicked,this,[this] { changedStars(5); });

    loadingMovie = new QMovie(":/loading_icon.gif");

    plugins.addPluginsToMenu(*(ui->menuPlugins),
                             ([this] (auto p, auto s) {
        this->pluginTriggered(p,s);
    }),
                             ([this] (double p) {
        progressBar->setValue(floor(p));
        QCoreApplication::processEvents();
        return !flag_stop_load_everything;
    }));
    last_plugin = std::pair<MillaGenericPlugin*,QAction*>(nullptr,nullptr);

    cleanUp();
    updateTags();
}

MViewer::~MViewer()
{
    if (loadingMovie) delete loadingMovie;
    QSqlDatabase::database().close();
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
    flag_stop_load_everything = false;

    if (finish) {
        QApplication::restoreOverrideCursor();
        loadingMovie->stop();
        if (loadingLabel) {
            ui->statusBar->removeWidget(loadingLabel);
            delete loadingLabel;
            loadingLabel = nullptr;
        }

    } else {
        QApplication::setOverrideCursor(Qt::WaitCursor);
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
    ThumbnailModel* ptr = new ThumbnailModel(lst,ui->listView);

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

    view_timer.start(20);
}

void MViewer::scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, QLabel* inflbl, double factor)
{
    if (!rec.valid) return;

    scaleFactor *= factor;
    if (scaleFactor <= FLT_EPSILON) scaleFactor = 1;

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

        if (ui->actionShow_faces->isChecked() && extr.valid && !extr.rois.empty()) {
            QImage inq(rec.picture.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation).toImage());
            QPainter painter(&inq);
            QPen paintpen(Qt::red);
            paintpen.setWidth(2);
            painter.setPen(paintpen);

            for (auto &i : extr.rois) {
                if (i.kind == MROI_FACE_FRONTAL)
                    painter.drawRect(QRect(i.x,i.y,i.w,i.h));
            }

            lbl->setPixmap(QPixmap::fromImage(inq));

        } else
            lbl->setPixmap(rec.picture.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }

}

unsigned MViewer::incViews(bool left)
{
    if ((left && !current_l.valid) || (!left && !current_r.valid)) return 0;
    QString fn = (left? current_l : current_r).filename;
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
            qDebug() << "history size now" << history.files.size() << "; we just pushed " << current_l.filename;
        }
    }

    return v;
}

void MViewer::leftImageMetaUpdate()
{
    if (current_l.valid) {
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

bool MViewer::isLoadableFile(QString const &path, QString *canonicalPath)
{
    QFileInfo cfn(path);
    QString ext(cfn.suffix().toLower());
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
        if (canonicalPath) *canonicalPath = cfn.canonicalFilePath();
        return true;
    }
    return false;
}

void MViewer::scanDirectory(QString const &dir, QStringList &addto, bool recursive)
{
    QDirIterator::IteratorFlags flags = QDirIterator::FollowSymlinks;
    if (recursive) flags |= QDirIterator::Subdirectories;
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, flags);
    QString cpath;
    while (it.hasNext()) {
        if (isLoadableFile(it.next(),&cpath)) addto.push_back(cpath);
    }
}

void MViewer::openDirByFile(QString const &fileName, bool recursive)
{
    if (fileName.isEmpty()) return;

    QFileInfo bpf(fileName);
    QString bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return;

    QStringList lst;
    scanDirectory(bpath,lst,recursive);
    showImageList(lst);
}

void MViewer::openDirByList(QString const &fileName)
{
    if (fileName.isEmpty()) return;

    QFile ilist(fileName);
    if (!ilist.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "ALERT: unable to read file " << fileName;
        return;
    }
    QString dat = ilist.readAll();
    ilist.close();

    QStringList ldat = dat.split('\n',QString::SkipEmptyParts);
    dat.clear();
    if (ldat.empty()) return;

    QStringList lst;
    QString cpath;
    for (auto &i : ldat) {
        if (i.isEmpty()) continue;
        if (i.at(i.size()-1) == '/') scanDirectory(i,lst,false);
        else if (i.right(2) == "/*") scanDirectory(i.left(i.size()-1),lst,true);
        else if (isLoadableFile(i,&cpath)) lst.push_back(cpath);
    }

    showImageList(lst);
}

void MViewer::processArguments()
{
    QStringList args = QApplication::arguments();
    if (args.size() < 2) return;
    args.erase(args.begin());

    if (args.size() == 1) {
        if (isLoadableFile(args.front(),nullptr)) openDirByFile(args.front());
        else {
            QFileInfo cfn(args.front());
            QString ext(cfn.suffix().toLower());
            if (ext == "txt" || ext == "lst")
                openDirByList(args.front());
        }

    } else {
        QStringList lst;
        QString cpath;
        for (auto &i : args) {
            if (isLoadableFile(i,&cpath)) lst.push_back(cpath);
        }
        showImageList(lst);
    }

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) {
        QString canon;
        if (isLoadableFile(args.front(),&canon))
            ui->listView->setCurrentIndex(ptm->getRecordIndex(canon));
        else
            ui->listView->setCurrentIndex(ptm->getRecordIndex(0));
    }
}

void MViewer::on_actionOpen_triggered()
{
    QString fn = QFileDialog::getOpenFileName(this, tr("Open image and directory"), "", tr(MILLA_OPEN_FILE));
    if (fn.isEmpty()) return;
    bool rec = QMessageBox::question(this, tr("Type of scan"), tr("Do recursive scan?"), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
    openDirByFile(fn,rec);
    on_actionDescending_triggered();
}

void MViewer::on_actionOpen_list_triggered()
{
    openDirByList(QFileDialog::getOpenFileName(this, tr("Open list of images"), "", tr(MILLA_OPEN_LIST)));
    on_actionDescending_triggered();
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
            return !flag_stop_load_everything;
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
        if (flag_stop_load_everything) break;
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
                                                 (flag_stop_load_everything?"Cancelled by user":"Finished")));
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

    if (!current_l.valid) {
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

    if (!ui->actionGlobal_search->isChecked() && ptm) {
        QList<MImageListRecord> &imgs = ptm->GetAllImages();
        for (auto &i : lst) {
            int idx = 0;

            for (auto &j : imgs) {
                if (j.filename == i) break;
                idx++;
            }
            if (idx >= imgs.size()) {
                qDebug() << "ALERT: Unable to match filename " << i;
                return;
            }

            ptm->LoadUp(idx);
            out.push_back(imgs.at(idx));
        }

    } else {
        for (auto &i : lst) {
            MImageListRecord r;
            r.filename = i;
            out.push_back(r);
        }
    }

    view->setModel(new SResultModel(out,view));
    view->setViewMode(QListView::ListMode);
    view->setFlow(QListView::LeftToRight);
    view->setWrapping(false);
    view->setWordWrap(true);

    ui->tabWidget->setCurrentIndex(tabIndex);
}

void MViewer::searchResults(QStringList lst)
{
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
    if (!frm.exec()) return;
    SearchFormData flt = frm.getSearchData();

    MImageListModel* ptm;
    if (flt.linked_only)
        ptm = dynamic_cast<MImageListModel*>(ui->listView_3->model());
    else
        ptm = dynamic_cast<MImageListModel*>(ui->listView_2->model()? ui->listView_2->model() : ui->listView->model());
    if (!ptm) return;

    searchResults(db.parametricSearch(flt,ptm->GetAllImages()));
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
}

void MViewer::on_actionQuit_triggered()
{
    QApplication::quit();
}

void MViewer::on_actionLink_left_to_right_triggered()
{
    if (!current_l.valid || !current_r.valid) return;

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

    QStringList out = db.getLinkedImages(extr.sha);
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
    if (current_l.valid)
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
    if (!to.valid) return;
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

    //create importer
    MImpExpModule mod(&tags_cache,(ptm? &(ptm->GetAllImages()) : nullptr));
    mod.setProgressBar([this] (double v) {
        this->progressBar->setValue(floor(v));
        QCoreApplication::processEvents();
        return !flag_stop_load_everything;
    });

    //and start the process
    prepareLongProcessing();
    bool ok = import?
                mod.dataImport(s,strm,[this,s] (auto fn) { return this->createStatRecord(fn,!s.loaded_only); }) :
                mod.dataExport(s,strm);

    //restore tags view
    if (current_l.valid) updateTags(current_l.filename);
    else updateTags();

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
    for (int i = 0; i < m && !flag_stop_load_everything; i++) {
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
        if (flag_stop_load_everything) break;

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
        return !flag_stop_load_everything;
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
        //load arbitary file using ThumbnailModel instance
        QStringList ls = { (*history.cur) };
        ptm = new ThumbnailModel(ls);
        ptm->LoadUp(0);
        current_l = ptm->data(ptm->getRecordIndex(0),MImageListModel::FullDataRole).value<MImageListRecord>();
        delete ptm;
        scaleImage(current_l,ui->scrollArea,ui->label,ui->label_3,1);
        leftImageMetaUpdate();
    }
}

void MViewer::on_actionRandom_image_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm || ptm->GetAllImages().empty()) return;
    double idx = (double)random() / (double)RAND_MAX * (double)(ptm->GetAllImages().size());
    qDebug() << "Randomly selecting item " << idx;
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
        return !flag_stop_load_everything;
    });

    bool abrt = flag_stop_load_everything;
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
    if (in.isNull()) return; //skip of no result were given

    //set current right image to generated one
    current_r = MImageListRecord();
    current_r.fnshort = "Generated Picture";
    current_r.picture = in;
    current_r.loaded = true;
    current_r.valid = true;

    //and show it
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,ui->label_4,1);
}

void MViewer::pluginTriggered(MillaGenericPlugin* plug, QAction* sender)
{
    QPixmap out;

    last_plugin = std::pair<MillaGenericPlugin*,QAction*>(plug,sender);

    prepareLongProcessing();
    if (plug->isFilter() && current_l.valid) { //Filter plugin

        //determine if we need to show UI for this plugin now
        if (!ui->actionAlways_show_GUI->isChecked()) {
            QVariant g(plug->getParam("show_ui"));
            if (g.canConvert<bool>() && g.value<bool>()) plug->showUI();
        } else
            plug->showUI();
        //ok, let's fire up some action
        QVariant r(plug->action(current_l.picture));
        //and present the result to the user
        if (r.canConvert<QPixmap>()) out = r.value<QPixmap>();

    } else if (!plug->isFilter()) { //Generator plugin

        //if generator is continous, check if it is already enabled
        if (plug->isContinous()) {
            if (!sender->isChecked()) { //since check was toggled before this call, check is inverted
                //stop it
                if (plugins_timers.count(plug)) {
                    plugins_timers[plug].stop();
                    disconnect(&(plugins_timers[plug]),&QTimer::timeout,nullptr,nullptr);
                    plugins_timers.erase(plug);
                }
                qDebug() << "[PLUGINS] " << plug->getPluginName() << " stopped";

            } else {
                //start it
                QVariant d(plug->getParam("update_delay"));
                int di = (d.canConvert<int>())? d.value<int>() : 0;
                if (di > 0) {
                    qDebug() << "[PLUGINS] Starting timer with interval " << di;
                    plugins_timers[plug].start(di); //timer created automatically by std::map
                    connect(&(plugins_timers[plug]),&QTimer::timeout,this,[plug,this] { this->pluginTimedOut(plug); });

                    //if plugin is timed, skip everything down there
                    prepareLongProcessing(true);
                    return;

                } else
                    qDebug() << "[PLUGINS] No update interval defined for " << plug->getPluginName();

                qDebug() << "[PLUGINS] " << plug->getPluginName() << " started";
            }
        }

        //grab the size of actual screen space (in 1:1 scale) available for generated picture
        QSize sz(ui->scrollArea_2->width(),ui->scrollArea_2->height());
        //and fire up the generation process
        QVariant r(plug->action(sz));

        //grab the result if it's available and valid
        if (r.canConvert<QPixmap>()) out = r.value<QPixmap>();
    }
    prepareLongProcessing(true);
    showGeneratedPicture(out);
}

void MViewer::pluginTimedOut(MillaGenericPlugin* plug)
{
    qDebug() << "[PLUGINS] timeout for " << plug->getPluginName();

    //TODO
}
