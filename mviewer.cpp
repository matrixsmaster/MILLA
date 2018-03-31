#include "mviewer.h"
#include "ui_mviewer.h"
#include "searchform.h"

MViewer::MViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MViewer)
{
    ui->setupUi(this);

    if (!initDatabase()) {
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

    using namespace cv;

    face_cascade = NULL;
    QFile test(FACE_CASCADE_FILE);
    if (test.exists()) test.remove();
    if (QFile::copy(":/face_cascade.xml",FACE_CASCADE_FILE)) {
        face_cascade = new CascadeClassifier();
        if (!face_cascade->load(FACE_CASCADE_FILE)) {
            qDebug() << "Unable to load cascade from " << FACE_CASCADE_FILE;
            delete face_cascade;
            face_cascade = NULL;
        }
        test.remove();
    }
}

MViewer::~MViewer()
{
    if (face_cascade) delete face_cascade;
    QSqlDatabase::database().close();
    delete ui;
}

bool MViewer::initDatabase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");

    QString fn = QDir::homePath() + DB_FILEPATH;
    qDebug() << "Storage database filename: " << fn;
    db.setHostName("localhost");
    db.setDatabaseName(fn);
    db.setUserName("user");
    bool ok = db.open();
    qDebug() << "DB open:  " << ok;
    if (!ok) return false;

    QSqlQuery q;
    ok = q.exec("SELECT version FROM meta") && q.next();
    qDebug() << "[db] Read meta table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE meta (" DBF_META ")");
        qDebug() << "[db] Create new meta table: " << ok;
        qq.clear();
        qq.prepare("INSERT INTO meta (version) VALUES (:ver)");
        qq.bindValue(":ver",DB_VERSION);
        ok = qq.exec();
        qDebug() << "[db] Inserting current DB version tag " << DB_VERSION << ": " << ok;
        if (!ok) return false;
    } else
        qDebug() << "DB version: " << q.value(0).toInt();

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'stats'");
    qDebug() << "[db] Read stats table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE stats (" DBF_STATS ")");
        qDebug() << "[db] Create new stats table: " << ok;
        if (!ok) return false;
    }

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'tags'");
    qDebug() << "[db] Read tags table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE tags (" DBF_TAGS ")");
        qDebug() << "[db] Create new tags table: " << ok;
        if (!ok) return false;
    } else
        updateTags();

    q.clear();
    ok = DB_CORRECT_TABLE_CHECK(q,"'links'");
    qDebug() << "[db] Read links table: " << ok;
    if (!ok) {
        QSqlQuery qq;
        ok = qq.exec("CREATE TABLE links (" DBF_LINKS ")");
        qDebug() << "[db] Create new links table: " << ok;
        if (!ok) return false;
    }

    return ok;
}

void MViewer::addTag(QString const &tg, int key, bool check)
{
    ui->listWidget->addItem(tg);
    QListWidgetItem* i = ui->listWidget->item(ui->listWidget->count()-1);
    if (!i) return;
    i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
    i->setCheckState(check? Qt::Checked : Qt::Unchecked);
    tags_cache[tg] = std::pair<int,bool>(key,check);
}

void MViewer::updateTags(QString fn)
{
    QSqlQuery q,qq;
    bool ok = q.exec("SELECT tag, key FROM tags ORDER BY rating DESC");
    qDebug() << "[db] Read whole tags table: " << ok;
    if (!ok) return;

    ui->listWidget->clear();
    tags_cache.clear();

    QStringList tlst;
    if (!fn.isEmpty()) {
        qq.prepare("SELECT tags FROM stats WHERE file = :fn");
        qq.bindValue(":fn",fn);
        if (qq.exec() && qq.next())
            tlst = qq.value(0).toString().split(',',QString::SkipEmptyParts);
    }

    bool c;
    while (q.next()) {
        c = (!tlst.empty() && tlst.contains(q.value(1).toString()));
        addTag(q.value(0).toString(),q.value(1).toInt(),c);
    }
}

void MViewer::updateStars(QString fn)
{
    int n = 5;

    if (!fn.isEmpty()) {
        QSqlQuery q;
        q.prepare("SELECT rating FROM stats WHERE file = :fn");
        q.bindValue(":fn",fn);
        if (q.exec() && q.next()) n = q.value(0).toInt();
        else n = 0;
    }

    ui->star_1->setStarActivated(n > 0);
    ui->star_2->setStarActivated(n > 1);
    ui->star_3->setStarActivated(n > 2);
    ui->star_4->setStarActivated(n > 3);
    ui->star_5->setStarActivated(n > 4);
}

void MViewer::changedStars(int n)
{
    if (!current_l.valid) return;
    if (n < 0) n = 0;
    if (n > 5) n = 5;

    QSqlQuery q;
    q.prepare("UPDATE stats SET rating = :r WHERE file = :fn");
    q.bindValue(":r",n);
    q.bindValue(":fn",current_l.filename);
    bool ok = q.exec();
    qDebug() << "[db] Rating update: " << ok;

    updateStars(current_l.filename);
}

void MViewer::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty() || ui->lineEdit->text().contains(',')) return;

    QSqlQuery q;
    q.prepare("SELECT * FROM tags WHERE tag = (:tg)");
    q.bindValue(":tg",ui->lineEdit->text());
    if (q.exec() && q.next()) {
        qDebug() << "[db] tag is already defined";
        return;
    }

    q.clear();
    if (!q.exec("SELECT MAX(key) FROM tags") || !q.next()) {
        qDebug() << "[db] ERROR: unable to get max key value from tags table";
        return;
    }
    unsigned key = q.value(0).toUInt() + 1;

    q.clear();
    q.prepare("INSERT INTO tags (key, tag, rating) VALUES (:k, :tg, 0)");
    q.bindValue(":k",key);
    q.bindValue(":tg",ui->lineEdit->text());
    bool ok = q.exec();

    qDebug() << "[db] Inserting tag " << ui->lineEdit->text() << ": " << ok;
    if (ok) {
        addTag(ui->lineEdit->text(),key);
        ui->lineEdit->clear();
    }
}

void MViewer::createStatRecord(QString fn)
{
    QSqlQuery qa;
    qa.prepare("SELECT views FROM stats WHERE file = (:fn)");
    qa.bindValue(":fn",fn);
    if (qa.exec() && qa.next()) {
        qDebug() << "[db] createStatRecord() called for known record " << fn;
        return;
    }

    MImageExtras ext = getExtraCacheLine(fn,true);
    if (!ext.valid) qDebug() << "[ALERT] INVALID EXTRA DATA RETURNED FOR " << fn;

    int fcn = 0;
    QString fcdat;
    for (auto &i : ext.rois)
        if (i.kind == MROI_FACE_FRONTAL) {
            fcn++;
            fcdat += QString::asprintf("%d,%d,%d,%d,",i.x,i.y,i.w,i.h);
        }

    QByteArray harr = qCompress(storeMat(ext.hist));
    qDebug() << "[db] Final harr length =" << harr.size();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    QByteArray shasum;
    QFile mfile(fn);
    mfile.open(QIODevice::ReadOnly);
    if (hash.addData(&mfile))
        shasum = hash.result();
    else
        qDebug() << "ALERT: Unable to calculate SHA256 of file " << fn;
    mfile.close();

    QSqlQuery q;
    q.prepare("INSERT INTO stats (" DBF_STATS_SHORT ") VALUES "
              "(:fn, 0, :tm, 0, 0, 0, \"\", \"\", :sx, :sy, :gry, :fcn, :fcr, :hst, :sha, :len)");
    q.bindValue(":fn",fn);
    q.bindValue(":tm",(uint)time(NULL));
    q.bindValue(":sx",ext.picsize.width());
    q.bindValue(":sy",ext.picsize.height());
    q.bindValue(":gry",ext.color? 0:1);
    q.bindValue(":fcn",fcn);
    q.bindValue(":fcr",fcdat);
    q.bindValue(":hst",harr);
    q.bindValue(":sha",shasum);
    q.bindValue(":len",mfile.size());

    bool ok = q.exec();
    qDebug() << "[db] Creating new statistics record: " << ok;
    checkExtraCache();
}

unsigned MViewer::incViews(bool left)
{
    if ((left && !current_l.valid) || (!left && !current_r.valid)) return 0;
    QString fn = (left? current_l : current_r).filename;
    qDebug() << "Incrementing views counter for " << fn;

    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);

    unsigned v = 0;
    if (q.exec() && q.next()) v = q.value(0).toUInt();
    else createStatRecord(fn);
    v++;

    q.clear();
    q.prepare("UPDATE stats SET views = :v, lastview = :tm WHERE file = :fn");
    q.bindValue(":v",v);
    q.bindValue(":tm",(uint)time(NULL));
    q.bindValue(":fn",fn);
    bool ok = q.exec();
    qDebug() << "[db] Updating views: " << ok;
    if (!ok) qDebug() << "[db] " << q.lastError().text();

    return v;
}

void MViewer::leftImageMetaUpdate()
{
    if (current_l.valid) {
        updateTags(current_l.filename);
        updateStars(current_l.filename);
        if (ui->actionShow_linked_image->isChecked())
            displayLinkedImages(current_l.filename);

        QSqlQuery q;
        q.prepare("SELECT views, notes FROM stats WHERE file = :fn");
        q.bindValue(":fn",current_l.filename);
        if (q.exec() && q.next()) {
            ui->lcdNumber->display((double)q.value(0).toUInt());
            ui->plainTextEdit->setPlainText(q.value(1).toString());
        } else {
            ui->lcdNumber->display(0);
            ui->plainTextEdit->clear();
        }
    }
    kudos(current_l,0);

    ui->radio_settags->setChecked(true);
    progressBar->setValue(0);
    ui->statusBar->showMessage("");
}

void MViewer::showNextImage()
{
    QModelIndex idx = ui->listView->selectionModel()->selectedIndexes().first();
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->LoadUp(idx.row());
    else return;
    ptm->touch(idx);

    current_l = idx.data(MImageListModel::FullDataRole).value<MImageListRecord>();
    scaleImage(current_l,ui->scrollArea,ui->label,1);
    leftImageMetaUpdate();
    checkExtraCache();

    view_timer.start(20);
}

void MViewer::cleanUp()
{
    extra_cache.clear();
    current_l = MImageListRecord();
    current_r = MImageListRecord();
}

void MViewer::checkExtraCache()
{
    if (extra_cache.size() > EXTRA_CACHE_SIZE) extra_cache.clear();
}

void MViewer::showImageList(QList<QString> const &lst)
{
    if (ui->listView->model()) {
        qDebug() << "Old model scheduled for removal";
        ui->listView->model()->deleteLater();
    }

    bool purelist = !(ui->actionThumbnails_cloud->isChecked());
    ThumbnailModel* ptr = new ThumbnailModel(lst,ui->listView);

    ptr->setShortenFilenames(!purelist);
    ui->listView->setModel(ptr);
    ui->listView->setViewMode(purelist? QListView::ListMode : QListView::IconMode);
    ui->listView->setFlow(purelist? QListView::TopToBottom : QListView::LeftToRight);
    ui->listView->setWrapping(!purelist);
    ui->listView->setSpacing(purelist? 5:10);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView->selectionModel(),&QItemSelectionModel::selectionChanged,[this] { showNextImage(); });
    connect(ui->listView,&QListView::customContextMenuRequested,this,[this] {
        current_r = ui->listView->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
        incViews(false);
    });

    ui->statusBar->showMessage(QString::asprintf("%d images",lst.size()));
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

void MViewer::scanDirectory(QString const &dir, QList<QString> &addto)
{
    QDirIterator it(dir, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    QString cpath;
    while (it.hasNext()) {
        if (isLoadableFile(it.next(),&cpath)) addto.push_back(cpath);
    }
}

void MViewer::openDirByFile(QString const &fileName)
{
    if (fileName.isEmpty()) return;

    QFileInfo bpf(fileName);
    QString bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return;

    QList<QString> lst;
    cleanUp();
    scanDirectory(bpath,lst);
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

    cleanUp();

    QList<QString> lst;
    QString cpath;
    for (auto &i : ldat) {
        if (!i.isEmpty() && i.at(i.size()-1) == '/') scanDirectory(i,lst);
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

        cleanUp();

        QList<QString> lst;
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
    openDirByFile(QFileDialog::getOpenFileName(this, tr("Open image and directory"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)")));
}

void MViewer::on_actionOpen_list_triggered()
{
    openDirByList(QFileDialog::getOpenFileName(this, tr("Open list of images"), "", tr("Text Files [txt,lst] (*.txt *.lst)")));
}

void MViewer::scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, double factor)
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

void MViewer::on_actionFit_triggered()
{
    if (!ui->actionFit->isChecked()) scaleFactor = 1;
    scaleImage(current_l,ui->scrollArea,ui->label,1);
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
}

QString MViewer::timePrinter(double sec) const
{
    QString res;
    double ehr = floor(sec / 3600.f);
    if (ehr > 0) {
        res += QString::asprintf("%.0f hrs ",ehr);
        sec -= ehr * 3600.f;
    }
    double emn = floor(sec / 60.f);
    if (emn > 0) {
        res += QString::asprintf("%.0f min ",emn);
        sec -= emn * 60.f;
    }
    res += QString::asprintf("%.1f sec",sec);
    return res;
}

cv::Mat MViewer::quickConvert(QImage &in) const //FIXME: not always working
{
    if (in.format() != QImage::Format_RGB888) {
        in = in.convertToFormat(QImage::Format_RGB888);
        qDebug() << "converting";
    }
    return cv::Mat(in.size().height(),in.size().width(),CV_8UC3,in.bits());
}

cv::Mat MViewer::slowConvert(QImage const &in) const
{
    using namespace cv;

    QImage n;
    if (in.format() != QImage::Format_RGB888) {
        qDebug() << "converting";
        n = in.convertToFormat(QImage::Format_RGB888);
    } else
        n = in;

    Mat r(n.size().height(),n.size().width(),CV_8UC3);
    for (int j,i = 0; i < r.rows; i++) {
        uchar* ptr = n.scanLine(i);
        for (j = 0; j < r.cols; j++) {
            r.at<Vec3b>(Point(j,i)) = Vec3b(*(ptr+2),*(ptr+1),*(ptr));
            ptr += 3;
        }
    }

    return r;
}

QByteArray MViewer::storeMat(cv::Mat const &in) const
{
    QByteArray harr;
    harr.append((char*)&(in.cols),sizeof(in.cols));
    harr.append((char*)&(in.rows),sizeof(in.rows));
    harr.append((char*)&(in.dims),sizeof(in.dims));

    int typ = in.type();
    harr.append((char*)&(typ),sizeof(typ));

    for (int i = 0; i < in.dims; i++)
        harr.append((char*)&(in.size[i]),sizeof(in.size[i]));

    size_t tmp = in.elemSize() * in.total();
    harr.append((char*)&tmp,sizeof(tmp));
    harr.append((char*)in.ptr(),tmp);

    return harr;
}

cv::Mat MViewer::loadMat(QByteArray const &arr) const
{
    cv::Mat res;
    const char* ptr = arr.constData();
    const int* iptr = (const int*)ptr;
    const size_t* uptr;

    int cols = *iptr++;
    int rows = *iptr++;
    int dims = *iptr++;
    int type = *iptr++;

    if (rows < 0 && cols < 0) {
        res.create(dims,iptr,type);
        iptr += dims;
    } else
        res.create(rows,cols,type);

    uptr = (const size_t*)iptr;
    size_t tot = *uptr;
    if (tot != res.elemSize() * res.total()) {
        qDebug() << "[db] ALERT: matrix size invalid";
        return res;
    }
    uptr++;

    ptr = (const char*)uptr;
    memcpy(res.ptr(),ptr,tot);

    return res;
}

void MViewer::on_actionMatch_triggered()
{
    using namespace cv;

    if (!current_l.valid || !ui->listView->model()) return;

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    MImageExtras orig = getExtraCacheLine(current_l.filename);
    if (!orig.valid) return;
    QSize orig_size = current_l.picture.size();
    double orig_area = orig_size.width() * orig_size.height();

    MImageExtras cur;
    std::map<double,MImageListRecord*> targets;

    for (auto &i : ptm->GetAllImages()) {
        if (current_l.filename == i.filename) continue;

        cur = getExtraCacheLine(i.filename);
        if (!cur.valid) continue;

        double cur_area = cur.picsize.width() * cur.picsize.height();
        if (orig_area / cur_area > 2 || cur_area / orig_area > 2) continue;

        double corr = compareHist(orig.hist,cur.hist,CV_COMP_CORREL);
        qDebug() << "Correlation with " << i.filename << ":  " << corr;

        if (corr > 0) targets[corr] = &i;
    }

    QList<QString> lst;
    int k = 0;
    for (auto i = targets.rbegin(); i != targets.rend() && k < 10; ++i,k++) { //TODO: move k into settings
        lst.push_back(i->second->filename);
    }

    searchResults(lst);
}

MImageExtras MViewer::getExtraCacheLine(QString const &fn, bool forceload)
{
    using namespace cv;

    if (extra_cache.count(fn))
        return extra_cache[fn];

    MImageExtras res;
    QSqlQuery q;
    q.prepare("SELECT sizex, sizey, grayscale, faces, facerects, hist, sha256 FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);;
    if (q.exec() && q.next()) {
        res.picsize = QSize(q.value(0).toUInt(), q.value(1).toUInt());
        res.color = !(q.value(2).toInt());
        QStringList flst = q.value(4).toString().split(',',QString::SkipEmptyParts);
        for (auto k = flst.begin(); k != flst.end();) {
            MROI r;
            r.kind = MROI_FACE_FRONTAL;
            r.x = k->toInt(); k++;
            r.y = k->toInt(); k++;
            r.w = k->toInt(); k++;
            r.h = k->toInt(); k++;
            res.rois.push_back(r);
        }
        if (q.value(5).canConvert(QVariant::ByteArray))
            res.hist = loadMat(qUncompress(q.value(5).toByteArray()));
        if (q.value(6).canConvert(QVariant::ByteArray))
            res.sha = q.value(6).toByteArray();

    } else {
        QPixmap org;

        //retreive image to analyze
        ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
        if (ptm) {
            size_t idx = 0;
            for (auto &i : ptm->GetAllImages()) {
                if (i.filename == fn) {
                    if (forceload) ptm->LoadUp(idx);
                    if (i.loaded) org = i.picture;
                    break;
                }
                idx++;
            }

        } else if (forceload) {
            org.load(fn);

        }
        if (org.isNull()) return res;

        //convert Pixmap into Mat
        QImage orgm(org.toImage());
        if (orgm.isNull()) return res;
        Mat in = slowConvert(orgm);

        res.picsize = org.size();

        //image histogram (3D)
        int histSize[] = {64, 64, 64};
        float rranges[] = {0, 256};
        const float* ranges[] = {rranges, rranges, rranges};
        int channels[] = {0, 1, 2};
        calcHist(&in,1,channels,Mat(),res.hist,3,histSize,ranges,true,false);

        res.color = false;
        //grayscale detection: fast approach
        for (int k = 0; k < res.hist.size[0]; k++) {
            float a = res.hist.at<float>(k,0,0);
            float b = res.hist.at<float>(0,k,0);
            float c = res.hist.at<float>(0,0,k);
            if (a != b || b != c || c != a) {
                res.color = true;
                break;
            }
        }
        if (!res.color && in.isContinuous()) {
            //grayscale detection: slow approach, as we're still not completely sure
            uchar* _ptr = in.ptr();
            for (int k = 0; k < in.rows && !res.color; k++)
                for (int kk = 0; kk < in.cols && !res.color; kk++) {
                    if (_ptr[0] != _ptr[1] || _ptr[1] != _ptr[2] || _ptr[2] != _ptr[0]) {
                        res.color = true;
                        qDebug() << "[grsdetect] Deep scan mismatch: " << _ptr[0] << _ptr[1] << _ptr[2];
                    }
                    _ptr += 3;
                }
        }

        //face detector
        std::vector<cv::Rect> faces;
        detectFaces(in,&faces);
        for (auto &i : faces) {
            MROI roi;
            roi.kind = MROI_FACE_FRONTAL;
            roi.x = i.x;
            roi.y = i.y;
            roi.w = i.width;
            roi.h = i.height;
            res.rois.push_back(roi);
        }
    }

    res.valid = true;
    extra_cache[fn] = res;
    return res;
}

void MViewer::detectFaces(const cv::Mat &inp, std::vector<cv::Rect>* store)
{
    using namespace cv;

    if (!face_cascade) return;

    std::vector<Rect> items;
    Mat work;

    try {
        cvtColor(inp,work,CV_BGR2GRAY);
        equalizeHist(work,work);
        face_cascade->detectMultiScale(work,items,1.1,3,0,Size(32,32));
    } catch (...) {
        qDebug() << "Error";
        return;
    }
    qDebug() << items.size() << " faces detected";

    if (store) store->insert(store->begin(),items.begin(),items.end());

    return;
}

void MViewer::on_actionLoad_everything_slow_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    stopButton->setEnabled(true);
    flag_stop_load_everything = false;

    double passed = 0, spd = 0, est, prg = 0, all = (double)(ptm->GetAllImages().size());
    double dp = 100.f / all;
    size_t k = 0;

    using namespace std::chrono;
    auto start = steady_clock::now();

    for (auto &i : ptm->GetAllImages()) {
        createStatRecord(i.filename);

        prg += dp;
        passed = (duration_cast<duration<double>>(steady_clock::now() - start)).count();
        spd = (double)(k++) / ((passed < FLT_EPSILON)? FLT_EPSILON : passed);
        est = (all - k) / spd;

        progressBar->setValue(floor(prg));
        ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %s; Processed: %lu; Speed: %.2f img/sec; Remaining: %s",
                                                     all,
                                                     timePrinter(passed).toStdString().c_str(),
                                                     k,spd,
                                                     timePrinter(est).toStdString().c_str()));

        QCoreApplication::processEvents();
        if (flag_stop_load_everything) break;
    }

    progressBar->setValue(100);
    ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %s; Speed: %.2f img/sec. %s.",
                                                 all,
                                                 timePrinter(passed).toStdString().c_str(),
                                                 spd,
                                                 (flag_stop_load_everything?"Cancelled by user":"Finished")));

    flag_stop_load_everything = false;
    stopButton->setEnabled(false);
}

void MViewer::on_listWidget_itemClicked(QListWidgetItem *item)
{
    if (!tags_cache.count(item->text())) {
        qDebug() << "ALERT: unknown tag " << item->text();
        return;
    }

    bool was = tags_cache[item->text()].second;
    bool now = item->checkState() == Qt::Checked;
    if (was == now) return;

    if (ui->radio_search->isChecked()) {
        //search by tag instead of change tags
        tags_cache[item->text()].second = now;
        searchByTag();
        return;
    }

    if (!current_l.valid) {
        //revert change
        item->setCheckState(was? Qt::Checked : Qt::Unchecked);
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT rating FROM tags WHERE tag = :tg");
    q.bindValue(":tg",item->text());
    if (q.exec() && q.next()) {
        uint rat = q.value(0).toUInt();
        if (now) rat++;
        else if (rat) rat--;
        tags_cache[item->text()].second = now;

        q.clear();
        q.prepare("UPDATE tags SET rating = :rat WHERE tag = :tg");
        q.bindValue(":rat",rat);
        q.bindValue(":tg",item->text());
        bool ok = q.exec();
        qDebug() << "[db] Updating rating to " << rat << " for tag " << item->text() << ":" << ok;

        updateCurrentTags();

    } else
        qDebug() << "ALERT: partially known tag " << item->text();
}

void MViewer::updateCurrentTags()
{
    if (!current_l.valid) return;

    QString tgs;
    int ntg = 0;
    for (auto &i : tags_cache)
        if (i.second.second) {
            tgs += QString::asprintf("%d,",i.second.first);
            ntg++;
        }

    QSqlQuery q;
    q.prepare("UPDATE stats SET ntags = :ntg, tags = :tgs WHERE file = :fn");
    q.bindValue(":ntg",ntg);
    q.bindValue(":tgs",tgs);
    q.bindValue(":fn",current_l.filename);
    bool ok = q.exec();
    qDebug() << "[db] Updating tags: " << ok;
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

void MViewer::resultsPresentation(QList<QString> lst, QListView* view, int tabIndex)
{
    if (view->model()) {
        qDebug() << "Old model scheduled for removal";
        view->model()->deleteLater();
    }

    if (lst.isEmpty()) return;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    QList<MImageListRecord> out;
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

    view->setModel(new SResultModel(out,view));
    view->setViewMode(QListView::ListMode);
    view->setFlow(QListView::LeftToRight);
    view->setWrapping(false);

    ui->tabWidget->setCurrentIndex(tabIndex);
}

void MViewer::searchResults(QList<QString> lst)
{
    resultsPresentation(lst,ui->listView_2,1);

    connect(ui->listView_2->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        MImageListRecord _r = ui->listView_2->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        if (ui->actionLeft_image->isChecked()) {
            current_l = _r;
            scaleImage(current_l,ui->scrollArea,ui->label,1);
            leftImageMetaUpdate();
            ui->lcdNumber->display((double)incViews());
        } else {
            current_r = _r;
            scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
            incViews(false);
        }
    });
}

void MViewer::searchByTag()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;
    QList<MImageListRecord> &imgs = ptm->GetAllImages();
    QSqlQuery q;
    std::map<QString,QList<int> > targ;
    QList<int> goal;
    QList<QString> found;

    for (auto &i : tags_cache) {
        if (!i.second.second) continue;
        goal.push_back(i.second.first);

        q.clear();
        q.prepare("SELECT file,tags FROM stats WHERE tags LIKE :t OR INSTR( tags, :i ) > 0");
        q.bindValue(":t",QString::asprintf("%d,%%",i.second.first));
        q.bindValue(":i",QString::asprintf(",%d,",i.second.first));
        if (!q.exec()) {
            qDebug() << "Select tag " << i.second.first << " failed";
            continue;
        }
        //qDebug() << q.lastQuery();

        while (q.next()) {
            qDebug() << "TAG " << i.second.first << " FOUND: " << q.value(0).toString();
            QList<int> l;
            QStringList _l = q.value(1).toString().split(",",QString::SkipEmptyParts);
            for (auto &j : _l) l.push_back(j.toInt());
            targ[q.value(0).toString()] = l;
        }
    }

    qDebug() << "Target list size: " << targ.size();

    for (auto &i : targ) {
        bool k = true;
        for (auto &j : goal)
            if (!i.second.contains(j)) {
                k = false;
                break;
            }
        if (!k) continue;

        auto w = std::find_if(imgs.begin(),imgs.end(),[&] (const MImageListRecord& a) { return (a.filename == i.first); });
        if (w == imgs.end()) {
            qDebug() << "File " << i.first << " isn't among currently loaded ones";
            continue;
        }

        found.push_back(i.first);
    }
    searchResults(found);
}

void MViewer::on_actionJump_to_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    bool ok;
    QString fn = QInputDialog::getText(this,tr("Jump to file"),tr("File name or full path"),QLineEdit::Normal,QString(),&ok);

    if (ok) ui->listView->setCurrentIndex(ptm->getRecordIndex(fn));
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

    QSqlQuery q;
    size_t area;
    std::multimap<size_t,QString> tmap;
    std::set<QString> tlst;

    for (auto &i : ptm->GetAllImages()) {
        q.clear();
        q.prepare("SELECT rating, likes, views, faces, grayscale, sizex, sizey, ntags, notes FROM stats WHERE file = :fn");
        q.bindValue(":fn",i.filename);
        if (q.exec() && q.next()) {
            if (flt.rating > q.value(0).toInt()) continue;
            if (flt.kudos > q.value(1).toInt()) continue;
            if (q.value(2).toUInt() < flt.minviews || q.value(2).toUInt() > flt.maxviews) continue;
            if (q.value(3).toInt() < flt.minface || q.value(3).toInt() > flt.maxface) continue;
            if (flt.colors > -1 && flt.colors != (q.value(4).toInt()>0)) continue;
            if (i.filechanged < flt.minmtime || i.filechanged > flt.maxmtime) continue;
            if (flt.wo_tags && q.value(7).toInt()) continue;
            if (flt.w_notes && q.value(8).toString().isEmpty()) continue;

            area = q.value(5).toUInt() * q.value(6).toUInt();
            if (area < flt.minsize || area > flt.maxsize) continue;

            switch (flt.sort) {
            case SRFRM_RATING: tmap.insert(std::pair<int,QString>(q.value(0).toInt(), i.filename)); break;
            case SRFRM_KUDOS: tmap.insert(std::pair<int,QString>(q.value(1).toInt(), i.filename)); break;
            case SRFRM_VIEWS: tmap.insert(std::pair<int,QString>(q.value(2).toUInt(), i.filename)); break;
            case SRFRM_DATE: tmap.insert(std::pair<int,QString>(i.filechanged, i.filename)); break;
            case SRFRM_FACES: tmap.insert(std::pair<int,QString>(q.value(3).toInt(), i.filename)); break;
            default: tlst.insert(i.filename);
            }
        }

        if (tlst.size() + tmap.size() >= flt.maxresults) break;
    }

    QList<QString> out;

    qDebug() << "Start of list";
    if (flt.sort == SRFRM_NAME) {
        for (auto &i : tlst) {
            qDebug() << i;
            out.push_back(i);
        }
    } else {
        for (auto i = tmap.rbegin(); i != tmap.rend(); ++i) {
            qDebug() << "key: " << i->first << "; val = " << i->second;
            out.push_back(i->second);
        }
    }
    qDebug() << "End of list";

    searchResults(out);
}

void MViewer::on_actionSwap_images_triggered()
{
    MImageListRecord tmp(current_l);
    current_l = current_r;
    current_r = tmp;
    scaleImage(current_l,ui->scrollArea,ui->label,1);
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
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

    QSqlQuery q;
    q.prepare("SELECT * FROM links WHERE left = :sl AND right = :sr");
    q.bindValue(":sl",extl.sha);
    q.bindValue(":sr",extr.sha);
    if (q.exec() && q.next()) {
        ui->statusBar->showMessage("Already linked");
        return;
    }

    q.clear();
    q.prepare("INSERT INTO links (left, right) VALUES (:sl, :sr)");
    q.bindValue(":sl",extl.sha);
    q.bindValue(":sr",extr.sha);
    bool ok = q.exec();

    qDebug() << "[db] Inserting link: " << ok;
    if (ok) ui->statusBar->showMessage("Linked");
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

    QSqlQuery q;
    q.prepare("SELECT right FROM links WHERE left = :sha");
    q.bindValue(":sha",extr.sha);
    if (!q.exec()) return;

    QList<QString> out;
    while (q.next()) {
        if (!q.value(0).canConvert(QVariant::ByteArray)) continue;

        QSqlQuery qq;
        qq.prepare("SELECT file FROM stats WHERE sha256 = :sha");
        qq.bindValue(":sha",q.value(0).toByteArray());
        if (qq.exec() && qq.next())
            out.push_back(qq.value(0).toString());
    }

    resultsPresentation(out,ui->listView_3,2);
    connect(ui->listView_3->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->listView_3->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
        incViews(false);
    });
}

void MViewer::on_actionAbout_triggered()
{
    QMessageBox::about(this, tr("About MILLA"),
                       tr("<p><b>MILLA</b> image viewer.</p>"
                          "<p><i>" MILLA_VERSION "</i></p>"
                          "<p><a href=" MILLA_SITE ">GitHub site</a></p>"
                          "<p>(C) Dmitry 'MatrixS_Master' Soloviov, 2018</p>"));
}

void MViewer::on_pushButton_2_clicked()
{
    if (!current_l.valid) return;

    QSqlQuery q;
    q.prepare("UPDATE stats SET notes = :nt WHERE file = :fn");
    q.bindValue(":nt",ui->plainTextEdit->toPlainText());
    q.bindValue(":fn",current_l.filename);
    bool ok = q.exec();
    qDebug() << "[db] Setting user notes: " << ok;
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

    int n = delta;
    QSqlQuery q;
    q.prepare("SELECT likes FROM stats WHERE file = :fn");
    q.bindValue(":fn",to.filename);

    if (!q.exec() || !q.next()) return;

    n += q.value(0).toInt();

    bool ok = true;
    if (delta) {
        q.clear();
        q.prepare("UPDATE stats SET likes = :lk WHERE file = :fn");
        q.bindValue(":lk",n);
        q.bindValue(":fn",to.filename);
        ok = q.exec();
        qDebug() << "[db] Updating likes: " << ok;
    }

    if (ok) ui->lcdNumber_2->display(n);
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

    QList<QString> lst;
    for (auto &i : ptm->GetAllImages()) lst.push_back(i.filename);

    cleanUp();
    showImageList(lst);
}

void MViewer::selectIEFileDialog(bool import)
{
    ExportForm frm(import);
    if (!frm.exec()) return;
    ExportFormData s = frm.getExportData();

    QString fileName = import?
                QFileDialog::getOpenFileName(this, tr("Import from"), "", tr("Text Files [txt,csv] (*.txt *.csv)")) :
                QFileDialog::getSaveFileName(this, tr("Export to"), "", tr("Text Files [txt,csv] (*.txt *.csv)"));
    if (fileName.isEmpty()) return;

    if ((!import) && (fileName.right(4).toUpper() != ".CSV" && fileName.right(4).toUpper() != ".TXT"))
        fileName += ".csv";

    QFile fl(fileName);
    if (!fl.open(QIODevice::Text | (import? QIODevice::ReadOnly : QIODevice::WriteOnly))) {
        qDebug() << "ALERT: unable to gain access to " << fileName;
        return;
    }

    QTextStream strm(&fl);
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    updateTags();

    MImpExpModule mod(&tags_cache,(ptm? &(ptm->GetAllImages()) : nullptr));
    mod.setProgressBar([this] (double v) {
        this->progressBar->setValue(floor(v));
        QCoreApplication::processEvents();
        return !flag_stop_load_everything;
    });

    stopButton->setEnabled(true);
    flag_stop_load_everything = false;
    progressBar->setValue(0);

    bool ok = import?
                mod.dataImport(s,strm,[this] (auto fn) { this->createStatRecord(fn); }) :
                mod.dataExport(s,strm);
    if (current_l.valid) updateTags(current_l.filename);

    progressBar->setValue(100);
    flag_stop_load_everything = false;
    stopButton->setEnabled(false);

    fl.close();
    qDebug() << "[db] " << (import? "Import":"Export") << " data: " << ok;

    if (!ok) QMessageBox::critical(this, tr("Import/Export error"), tr("Unable to finish operation"));
}

void MViewer::on_actionExport_data_triggered()
{
    selectIEFileDialog(false);
}

void MViewer::on_actionImport_data_triggered()
{
    selectIEFileDialog(true);
}
