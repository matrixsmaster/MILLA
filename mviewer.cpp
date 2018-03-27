#include "mviewer.h"
#include "ui_mviewer.h"
#include "searchform.h"

MViewer::MViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MViewer)
{
    ui->setupUi(this);
    scaleFactor = 1;

    progressBar = new QProgressBar(this);
    progressBar->setTextVisible(false);
    ui->statusBar->addPermanentWidget(progressBar);

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    QString fn = QDir::homePath() + "/.milla/storage.db";
    qDebug() << "Storage database filename: " << fn;
    db.setHostName("localhost");
    db.setDatabaseName(fn);
    db.setUserName("user");
    bool ok = db.open();
    qDebug() << "DB open:  " << ok;

    if (ok) {
        QSqlQuery q;
        ok = q.exec("SELECT * FROM meta");
        qDebug() << "[db] Read meta table: " << ok;
        if (!ok) {
            QSqlQuery qq;
            ok = qq.exec("CREATE TABLE meta (version INT)");
            qDebug() << "[db] Create new meta table: " << ok;
            qq.clear();
            qq.prepare("INSERT INTO meta (version) VALUES (:ver)");
            qq.bindValue(":ver",INTERNAL_DB_VERSION);
            ok = qq.exec();
            qDebug() << "[db] Inserting current DB version tag " << INTERNAL_DB_VERSION << ": " << ok;
        }

        q.clear();
        ok = q.exec("SELECT * FROM stats LIMIT 1");
        qDebug() << "[db] Read stats table: " << ok;
        if (!ok) {
            QSqlQuery qq;
            ok = qq.exec("CREATE TABLE stats (file TEXT, views UNSIGNED BIGINT, lastview UNSIGNED INT, rating TINYINT, likes INT, ntags INT, tags TEXT, notes TEXT, "
                         "sizex UNSIGNED INT, sizey UNSIGNED INT, grayscale TINYINT, faces INT, facerects TEXT, hist BLOB, sha256 BLOB, length UNSIGNED BIGINT)");
            qDebug() << "[db] Create new stats table: " << ok;
        }

        q.clear();
        ok = q.exec("SELECT tag FROM tags LIMIT 1");
        qDebug() << "[db] Read tags table: " << ok;
        if (!ok) {
            QSqlQuery qq;
            ok = qq.exec("CREATE TABLE tags (key UNSIGNED INT, tag TEXT, rating UNSIGNED BIGINT)");
            qDebug() << "[db] Create new tags table: " << ok;

        } else
            updateTags();

        q.clear();
        ok = q.exec("SELECT * FROM links LIMIT 1");
        qDebug() << "[db] Read links table: " << ok;
        if (!ok) {
            QSqlQuery qq;
            ok = qq.exec("CREATE TABLE links (left BLOB, right BLOB)");
            qDebug() << "[db] Create new links table: " << ok;
        }

    }
    if (!ok) qDebug() << "[ALERT] DB operations aren't finished";

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
    if (view_timer) delete view_timer;
    QSqlDatabase::database().close();
    delete ui;
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
    if (ok) {
        ui->listWidget->clear();
        tags_cache.clear();

        QStringList tlst;
        if (!fn.isEmpty()) {
            qq.prepare("SELECT tags FROM stats WHERE file = :fn");
            qq.bindValue(":fn",fn);
            if (qq.exec() && qq.next())
                tlst = qq.value(0).toString().split(',',QString::SkipEmptyParts);
        }

        while (q.next()) {
            bool c = false;
            if (!tlst.empty() && tlst.contains(q.value(1).toString())) c = true;
            addTag(q.value(0).toString(),q.value(1).toInt(),c);
        }
    }
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

#if 0
static std::string type2str(int type)
{
    std::string r;

    uchar depth = type & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (type >> CV_CN_SHIFT);

    switch ( depth ) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
    }

    r += "C";
    r += (chans+'0');

    return r;
}
#endif

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
    q.prepare("INSERT INTO stats (file, views, lastview, rating, likes, ntags, tags, notes, sizex, sizey, grayscale, faces, facerects, hist, sha256, length) VALUES "
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

    return v;
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
    updateTags(current_l.filename);

    progressBar->setValue(0);
    ui->radio_settags->setChecked(true);

    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = :fn");
    q.bindValue(":fn",current_l.filename);
    if (q.exec() && q.next())
        ui->lcdNumber->display((double)q.value(0).toUInt());
    else
        ui->lcdNumber->display(0);

    if (view_timer) view_timer->start(20);
    else {
        view_timer = new QTimer();
        connect(view_timer,&QTimer::timeout,this,[this] {
            if (progressBar->value() < 100)
                progressBar->setValue(progressBar->value()+1);
            else {
                view_timer->stop();
                ui->lcdNumber->display((double)incViews());
            }
        });
    }

    if (ui->actionShow_linked_image->isChecked())
        displayLinkedImages(current_l.filename);
}

void MViewer::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty()) return;

    extra_cache.clear();
    current_l = MImageListRecord();
    current_r = MImageListRecord();

    QFileInfo bpf(fileName);
    QString bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return;

    QDirIterator it(bpath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    QList<QString> lst;
    while (it.hasNext()) {
        QFileInfo cfn(it.next());
        QString ext(cfn.suffix().toLower());
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
            lst.push_back(cfn.canonicalFilePath());
    }

    if (ui->listView->model()) {
        qDebug() << "Old model scheduled for removal";
        ui->listView->model()->deleteLater();
    }

    bool purelist = true; //TODO: move it to user settings

    ui->listView->setModel(new ThumbnailModel(lst,ui->listView));
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
        qDebug() << "rightClick";
    });
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

cv::Mat MViewer::quickConvert(QImage &in) //FIXME: not always working
{
    if (in.format() != QImage::Format_RGB888) {
        in = in.convertToFormat(QImage::Format_RGB888);
        qDebug() << "converting";
    }
    return cv::Mat(in.size().height(),in.size().width(),CV_8UC3,in.bits());
}

cv::Mat MViewer::slowConvert(QImage const &in)
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

QByteArray MViewer::storeMat(cv::Mat const &in)
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

cv::Mat MViewer::loadMat(QByteArray const &arr)
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
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return res;

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
        size_t idx = 0;
        for (auto &i : ptm->GetAllImages()) {
            if (i.filename == fn) {
                if (forceload) ptm->LoadUp(idx);
                if (i.loaded) org = i.picture;
                break;
            }
            idx++;
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

void MViewer::on_actionLoad_all_known_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    size_t x = 0;
    for (auto &i : ptm->GetAllImages()) {
        if (!i.modified && i.picture.isNull()) ptm->LoadUp(x);
        x++;
    }
}

void MViewer::on_actionLoad_everything_slow_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    double passed, spd, est, prg = 0, all = (double)(ptm->GetAllImages().size());
    double dp = 100.f / all;
    time_t pass, start = clock();
    size_t k = 0;

    for (auto &i : ptm->GetAllImages()) {
        createStatRecord(i.filename);

        prg += dp;
        pass = clock() - start;
        passed = (double)pass / (double)CLOCKS_PER_SEC;
        spd = passed / (double)(k++);
        est = (all - k) / spd;

        progressBar->setValue(floor(prg));
        ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %.1f sec; Speed: %.2f img/sec; Remaining: %.1f sec",all,passed,spd,est));

        QCoreApplication::processEvents();
    }

    progressBar->setValue(100);
    ui->statusBar->showMessage(QString::asprintf("Objects: %.0f; Elapsed: %.1f sec; Speed: %.2f img/sec",all,passed,spd));
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
        current_r = ui->listView_2->selectionModel()->selectedIndexes().first().data(MImageListModel::FullDataRole).value<MImageListRecord>();
        scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
        incViews(false);
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
    if (!ok) return;
    bool path = fn.contains('/');

    size_t idx = 0;
    ok = false;
    for (auto &i : ptm->GetAllImages()) {
        if ((path && !fn.compare(i.filename,Qt::CaseInsensitive)) || (!path && !fn.compare(i.fnshort,Qt::CaseInsensitive))) {
            ok = true;
            current_l = i;
            break;
        }
        idx++;
    }

    if (ok) scaleImage(current_l,ui->scrollArea,ui->label,1);
}

void MViewer::on_actionRefine_search_triggered()
{
    SearchForm frm;
    if (!frm.exec()) return;
    SearchFormData flt = frm.getSearchData();

    MImageListModel* ptm = dynamic_cast<MImageListModel*>(ui->listView_2->model()? ui->listView_2->model() : ui->listView->model());
    if (!ptm) return;

    QSqlQuery q;
    QList<QString> out;
    for (auto &i : ptm->GetAllImages()) {
        q.clear();
        q.prepare("SELECT rating, views, faces, grayscale FROM stats WHERE file = :fn");
        q.bindValue(":fn",i.filename);
        if (q.exec() && q.next()) {
            if (q.value(0).toInt() && flt.rating > q.value(0).toInt()) continue;
            if (flt.views > q.value(1).toUInt()) continue;
            if (q.value(2).toInt() < flt.minface || q.value(2).toInt() > flt.maxface) continue;
            if (flt.grey != (q.value(3).toInt()>0)) continue;
            if (i.filechanged < flt.mtime_min || i.filechanged > flt.mtime_max) continue;

            out.push_back(i.filename);
        }
    }
    searchResults(out);
}

void MViewer::on_actionSwap_images_triggered()
{
    MImageListRecord tmp(current_l);
    current_l = current_r;
    current_r = tmp;
    scaleImage(current_l,ui->scrollArea,ui->label,1);
    scaleImage(current_r,ui->scrollArea_2,ui->label_2,1);
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
    if (!extl.valid || !extr.valid) return;

    QSqlQuery q;
    q.prepare("SELECT * FROM links WHERE left = :sl AND right = :sr");
    q.bindValue(":sl",extl.sha);
    q.bindValue(":sr",extr.sha);
    if (q.exec() && q.next()) {
        qDebug() << "[db] Link is already registered, exiting";
        return;
    }

    q.clear();
    q.prepare("INSERT INTO links (left, right) VALUES (:sl, :sr)");
    q.bindValue(":sl",extl.sha);
    q.bindValue(":sr",extr.sha);
    bool ok = q.exec();

    qDebug() << "[db] Inserting link: " << ok;
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
