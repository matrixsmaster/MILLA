#include "mviewer.h"
#include "ui_mviewer.h"

MViewer::MViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MViewer)
{
    ui->setupUi(this);
    scaleFactor = 1;

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
            ok = qq.exec("CREATE TABLE stats (file TEXT, views UNSIGNED BIGINT, lastview UNSIGNED INT, rating TINYINT, ntags INT, tags TEXT, notes TEXT, "
                         "sizex UNSIGNED INT, sizey UNSIGNED INT, grayscale TINYINT, faces INT, facerects TEXT, hist BLOB)");
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

    QSqlQuery q;
    q.prepare("INSERT INTO stats (file, views, lastview, rating, ntags, tags, notes, sizex, sizey, grayscale, faces, facerects, hist) VALUES "
              "(:fn, 0, :tm, 0, 0, \"\", \"\", :sx, :sy, -1, :fcn, :fcr, :hst)");
    q.bindValue(":fn",fn);
    q.bindValue(":tm",(uint)time(NULL));
    q.bindValue(":sx",ext.picsize.width());
    q.bindValue(":sy",ext.picsize.height());
    q.bindValue(":fcn",fcn);
    q.bindValue(":fcr",fcdat);
    q.bindValue(":hst",harr);

    bool ok = q.exec();
    qDebug() << "[db] Creating new statistics record: " << ok;
}

unsigned MViewer::incViews(bool left)
{
    if ((left && !current_l.isValid()) || (!left && !current_r.isValid())) return 0;
    QString fn = (left? current_l : current_r).data(ThumbnailModel::FullPathRole).value<QString>();
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
    current_l = ui->listView->selectionModel()->selectedIndexes().first();
    scaleImage(ui->scrollArea,ui->label,&current_l,1);

    QString fn = current_l.data(ThumbnailModel::FullPathRole).toString();
    qDebug() << fn;
    updateTags(fn);
    ui->progressBar->setValue(0);

    QSqlQuery q;
    q.prepare("SELECT views FROM stats WHERE file = :fn");
    q.bindValue(":fn",fn);
    if (q.exec() && q.next())
        ui->lcdNumber->display((double)q.value(0).toUInt());
    else
        ui->lcdNumber->display(0);

    if (view_timer) view_timer->start(20);
    else {
        view_timer = new QTimer();
        connect(view_timer,&QTimer::timeout,this,[this] {
            if (ui->progressBar->value() < 100)
                ui->progressBar->setValue(ui->progressBar->value()+1);
            else {
                view_timer->stop();
                ui->lcdNumber->display((double)incViews());
            }
        });
    }
    qDebug() << "selChanged";
}

void MViewer::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty()) return;

    extra_cache.clear();

    QFileInfo bpf(fileName);
    QString bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return;

    QDirIterator it(bpath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    std::list<QString> lst;
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
        current_r = ui->listView->selectionModel()->selectedIndexes().first();
        scaleImage(ui->scrollArea_2,ui->label_2,&current_r,1);
        incViews(false);
        qDebug() << "rightClick";
    });
}

void MViewer::scaleImage(QScrollArea* scrl, QLabel* lbl, QModelIndex *idx, double factor)
{
    if (!idx->isValid() || !ui->listView->model()) return;

    scaleFactor *= factor;
    if (scaleFactor <= FLT_EPSILON) scaleFactor = 1;

    scrl->setWidgetResizable(false);
    lbl->setPixmap(QPixmap());
    lbl->setText("");
    lbl->updateGeometry();
    scrl->updateGeometry();
    scrl->setWidgetResizable(true);

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (ptm) ptm->LoadUp(idx->row());
    else return;

    QPixmap map = idx->data(ThumbnailModel::LargePixmapRole).value<QPixmap>();

    if (ui->actionFit->isChecked())
        lbl->setPixmap(map.scaled(lbl->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));

    else {
        QSize nsz = map.size() * scaleFactor;
        MImageExtras extr = getExtraCacheLine(idx->data(ThumbnailModel::FullPathRole).value<QString>());

        if (ui->actionShow_faces->isChecked() && extr.valid && !extr.rois.empty()) {
            QImage inq(map.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation).toImage());
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
            lbl->setPixmap(map.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }

}

void MViewer::on_actionFit_triggered()
{
    if (!ui->actionFit->isChecked()) scaleFactor = 1;
    scaleImage(ui->scrollArea,ui->label,&current_l,1);
    scaleImage(ui->scrollArea_2,ui->label_2,&current_r,1);
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

    qDebug() << "[db] Size of harr = " << harr.size() << "; cols, rows, dims = " << in.cols << in.rows << in.dims;
    qDebug() << tmp;
    qDebug() << in.elemSize() << " <- " << type2str(in.type()).c_str();

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

    qDebug() << "[db] Size of arr = " << arr.size() << "; cols, rows, dims = " << res.cols << res.rows << res.dims;
    qDebug() << tot;
    qDebug() << res.elemSize() << " <- " << type2str(res.type()).c_str();

    return res;
}

void MViewer::on_actionMatch_triggered()
{
    using namespace cv;

    if (!current_l.isValid() || !ui->listView->model()) return;

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    MImageExtras orig = getExtraCacheLine(current_l.data(ThumbnailModel::FullPathRole).value<QString>());
    if (!orig.valid) return;
    QSize orig_size = current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>().size();
    double orig_area = orig_size.width() * orig_size.height();

    MImageExtras cur;
    std::map<double,ThumbnailRec*> targets;
    QString we = current_l.data(ThumbnailModel::FullPathRole).value<QString>();

    for (auto &i : ptm->GetAllImages()) {
        if (we == i->filename) continue;

        cur = getExtraCacheLine(i->filename);
        if (!cur.valid) continue;

        double cur_area = cur.picsize.width() * cur.picsize.height();
        if (orig_area / cur_area > 2 || cur_area / orig_area > 2) continue;

        double corr = compareHist(orig.hist,cur.hist,CV_COMP_CORREL);
        qDebug() << "Correlation with " << i->filename << ":  " << corr;

        if (corr > 0) targets[corr] = i;
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
    q.prepare("SELECT sizex, sizey, grayscale, faces, facerects, hist FROM stats WHERE file = (:fn)");
    q.bindValue(":fn",fn);;
    if (q.exec() && q.next()) {
        res.picsize = QSize(q.value(0).toUInt(), q.value(1).toUInt());
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

    } else {

        QPixmap org;
        size_t idx = 0;
        for (auto &i : ptm->GetAllImages()) {
            if (i->filename == fn) {
                if (forceload) ptm->LoadUp(idx);
                if (i->loaded) org = i->picture;
                break;
            }
            idx++;
        }
        if (org.isNull()) return res;

        QImage orgm(org.toImage());
        if (orgm.isNull()) return res;
        Mat in = slowConvert(orgm);

        res.picsize = org.size();

        int histSize[] = {64, 64, 64};
        float rranges[] = {0, 256};
        const float* ranges[] = {rranges, rranges, rranges};
        int channels[] = {0, 1, 2};
        calcHist(&in,1,channels,Mat(),res.hist,3,histSize,ranges,true,false);

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
        if (!i->modified && i->picture.isNull()) ptm->LoadUp(x);
        x++;
    }
}

void MViewer::on_actionLoad_everything_slow_triggered()
{
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    QList<ThumbnailRec*> &imgs = ptm->GetAllImages();
    double prg = 0, dp = 100.f / (double)(imgs.size());

    for (auto &i : ptm->GetAllImages()) {
        createStatRecord(i->filename);
        prg += dp;
        ui->progressBar->setValue(floor(prg));
        QCoreApplication::processEvents();
    }
    ui->progressBar->setValue(100);
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

    if (!current_l.isValid()) {
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
    if (!current_l.isValid()) return;

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
    q.bindValue(":fn",current_l.data(ThumbnailModel::FullPathRole).toString());
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
        if (current_l.isValid()) updateTags(current_l.data(ThumbnailModel::FullPathRole).toString());
        else updateTags();
    }
}

void MViewer::searchResults(QList<QString> lst)
{
    if (lst.isEmpty()) return;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    if (ui->listView_2->model()) {
        qDebug() << "Old model scheduled for removal";
        ui->listView_2->model()->deleteLater();
    }

    QList<SResultRecord> out;
    QList<ThumbnailRec*> &imgs = ptm->GetAllImages();
    for (auto &i : lst) {
        SResultRecord k;
        int idx = 0;

        k.path = i;
        for (auto &j : imgs) {
            if (j->filename == i) break;
            idx++;
        }
        if (idx >= imgs.size()) {
            qDebug() << "ALERT: Unable to match filename " << i;
            return;
        }
        k.shrt = imgs.at(idx)->fnshort;
        k.thumb = imgs.at(idx)->thumb;

        ptm->LoadUp(idx);
        k.large = imgs.at(idx)->picture;

        out.push_back(k);
    }

    ui->listView_2->setModel(new SResultModel(out,ui->listView_2));
    ui->listView_2->setViewMode(QListView::ListMode);
    ui->listView_2->setFlow(QListView::LeftToRight);
    ui->listView_2->setWrapping(false);

    connect(ui->listView_2->selectionModel(),&QItemSelectionModel::selectionChanged,[this] {
        current_r = ui->listView_2->selectionModel()->selectedIndexes().first();
        scaleImage(ui->scrollArea_2,ui->label_2,&current_r,1);
        incViews(false);
    });

    ui->tabWidget->setCurrentIndex(1);
}

void MViewer::searchByTag()
{
    QSqlQuery q;

    for (auto &i : tags_cache) {
        if (!i.second.second) continue;
        q.clear();
        q.prepare("SELECT file FROM stats WHERE tags LIKE :t OR INSTR( tags, :i ) > 0");
        q.bindValue(":t",QString::asprintf("%d,%%",i.second.first));
        q.bindValue(":i",QString::asprintf(",%d,",i.second.first));
        if (!q.exec()) {
            qDebug() << "Select tag " << i.second.first << " failed";
            continue;
        }
        //qDebug() << q.lastQuery();

        while (q.next()) {
            qDebug() << "TAG " << i.second.first << " FOUND: " << q.value(0).toString();
        }

    }

}
