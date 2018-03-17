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
        ok = q.exec("SELECT * FROM stats");
        qDebug() << "[db] Read stats table: " << ok;
        if (!ok) {
            QSqlQuery qq;
            ok = qq.exec("CREATE TABLE stats (file TEXT, views UNSIGNED BIGINT, rating TINYINT, tags TEXT, notes TEXT)");
            qDebug() << "[db] Create new stats table: " << ok;
        }
    }
}

MViewer::~MViewer()
{
    QSqlDatabase::database().close();
    delete ui;
}

void MViewer::on_pushButton_clicked()
{
    //TODO
}

void MViewer::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty()) return;
    qDebug() << fileName;

    match_cache.clear();

    QFileInfo bpf(fileName);
    QString bpath = bpf.canonicalPath();
    if (bpath.isEmpty()) return;

    QDirIterator it(bpath, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    std::list<QString> lst;
    while (it.hasNext()) {
        QFileInfo cfn(it.next());
        //qDebug() << cfn.fileName() << " -> " << cfn.suffix();
        QString ext(cfn.suffix().toLower());
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") {
            //qDebug() << "OK";
            lst.push_back(cfn.canonicalFilePath());
        }
    }

    if (ui->listView->model()) {
        qDebug() << "Old model scheduled for removal";
        ui->listView->model()->deleteLater();
    }

    ui->listView->setModel(new ThumbnailModel(lst,ui->listView));
    ui->listView->setViewMode(QListView::ListMode);
    ui->listView->setFlow(QListView::LeftToRight);
    ui->listView->setSelectionMode(QListView::SingleSelection);
    ui->listView->setWrapping(true);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
        current_l = ui->listView->selectionModel()->selectedIndexes().first();
        scaleImage(ui->scrollArea,ui->label,&current_l,1);
        //reinterpret_cast<ThumbnailModel*>(ui->listView->model())->GC();
        qDebug() << "selChanged";
    });
    connect(ui->listView, &QListView::customContextMenuRequested, this, [this] {
        current_r = ui->listView->selectionModel()->selectedIndexes().first();
        scaleImage(ui->scrollArea_2,ui->label_2,&current_r,1);
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
        lbl->setPixmap(map.scaled(nsz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }

}

void MViewer::on_actionFit_triggered()
{
    if (!ui->actionFit->isChecked()) scaleFactor = 1;
    scaleImage(ui->scrollArea,ui->label,&current_l,1);
    scaleImage(ui->scrollArea_2,ui->label_2,&current_r,1);
}

cv::Mat MViewer::quickConvert(QImage const &in)
{
#if 0
    if (in.format() != QImage::Format_RGB888) {
        in = in.convertToFormat(QImage::Format_RGB888);
        qDebug() << "converting";
    }
    return cv::Mat(in.size().height(),in.size().width(),CV_8UC3,in.bits());
#else
    using namespace cv;

    QImage n;
    if (in.format() != QImage::Format_RGB888) {
        qDebug() << "converting";
        n = in.convertToFormat(QImage::Format_RGB888);
    } else
        n = in;

    Mat r(n.size().height(),n.size().width(),CV_8UC3);
//    uchar* ptr = n.bits();
    for (int j,i = 0; i < r.rows; i++) {
        uchar* ptr = n.scanLine(i);
        for (j = 0; j < r.cols; j++) {
            r.at<Vec3b>(Point(j,i)) = Vec3b(*(ptr+2),*(ptr+1),*(ptr));
            ptr += 3;
        }
    }

    return r;
#endif
}

void MViewer::on_actionMatch_triggered()
{
    using namespace cv;

    if (!current_l.isValid() || !ui->listView->model()) return;

    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return;

    MMatcherCacheRec orig = getMatchCacheLine(current_l.data(ThumbnailModel::FullPathRole).value<QString>());
    if (!orig.valid) return;
    QSize orig_size = current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>().size();
    double orig_area = orig_size.width() * orig_size.height();

    FlannBasedMatcher matcher;
    size_t maxgoods = 0;
    ThumbnailRec* winner = nullptr;
    MMatcherCacheRec cur;
    Mat img_matches; //FIXME: debug only
    std::map<double,ThumbnailRec*> targets;
    QString we = current_l.data(ThumbnailModel::FullPathRole).value<QString>();

    for (auto &i : ptm->GetAllImages()) {
        if (we == i->filename) continue;

        if (!i->loaded) continue; //FIXME: debug only

        double cur_area = i->picture.size().width() * i->picture.size().height();
        if (orig_area / cur_area > 2 || cur_area / orig_area > 2) continue;

        cur = getMatchCacheLine(i->filename);
        if (!cur.valid) continue;

        double corr = compareHist(orig.hist,cur.hist,CV_COMP_CORREL);
        qDebug() << "Correlation with " << i->filename << ":  " << corr;

        if (corr > 0) targets[corr] = i;
    }

    if (!targets.empty())
        qDebug() << "Total targets: " << targets.size();
    else
        return;

#if 1
    int n = 0;
    for (auto i = targets.rbegin(); i != targets.rend() && n < 5; ++i,n++) {
        cur = getMatchCacheLine(i->second->filename);
        if (!cur.valid) continue;

        vector<DMatch> matches;
        try {
            matcher.match(orig.desc,cur.desc,matches);
        } catch (...) {
            qDebug() << "Matching error on " << i->second->fnshort;
        }

        double dist,max_dist = 0;
        double min_dist = 100;

        //calculate distance max && min
        for (int j = 0; j < orig.desc.rows; j++) {
            dist = matches[j].distance;
            if (dist < min_dist) min_dist = dist;
            if (dist > max_dist) max_dist = dist;
        }

        //matches filtering
        std::vector<DMatch> good_matches;
        for (int j = 0; j < orig.desc.rows; j++)
            if (matches[j].distance <= max(2*min_dist,0.02)) {
                good_matches.push_back(matches[j]);
            }
        qDebug() << "Total " << good_matches.size() << " good matches for " << i->second->fnshort;

        if (good_matches.size() > maxgoods && !cur.kpv.empty()) {
            maxgoods = good_matches.size();
            winner = i->second;

            //FIXME: debug only
            try {
                drawMatches( orig.tmp_img, orig.kpv, cur.tmp_img, cur.kpv,
                         good_matches, img_matches );
                         //Scalar::all(-1), Scalar::all(-1), vector<char>(), DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS );
            } catch (...) {
                qDebug() << "drawMatches error";
            }
        }
    }
#endif

    if (winner) {
        ui->label_2->setPixmap(winner->picture);

        //-- Show detected matches
        imshow( "Good Matches", img_matches );
    }
}

void MViewer::on_actionTest_triggered()
{
    if (!current_l.isValid() || !ui->listView->model()) return;
    QImage i(current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>().toImage());
    cv::Mat m(quickConvert(i));
    cv::imshow("TEST",m);
}

MMatcherCacheRec MViewer::getMatchCacheLine(QString const &fn)
{
    using namespace cv;

    if (match_cache.count(fn))
        return match_cache[fn];

    MMatcherCacheRec res;
    ThumbnailModel* ptm = dynamic_cast<ThumbnailModel*>(ui->listView->model());
    if (!ptm) return res;

    QPixmap org;
    for (auto &i : ptm->GetAllImages())
        if (i->filename == fn) {
            if (i->loaded) org = i->picture;
            else org.load(i->filename);
            break;
        }
    if (org.isNull()) return res;

    QImage orgm(org.toImage());
    if (orgm.isNull()) return res;
    Mat in = quickConvert(orgm);

    int histSize[] = {64, 64, 64};
    float rranges[] = {0, 256};
    const float* ranges[] = {rranges, rranges, rranges};
    int channels[] = {0, 1, 2};
    calcHist(&in,1,channels,Mat(),res.hist,3,histSize,ranges,true,false);

#if 1
    SurfFeatureDetector det(FLATS_MINHESSIAN);
    SurfDescriptorExtractor extr;

    try {
        det.detect(in,res.kpv);
        extr.compute(in,res.kpv,res.desc);
    } catch (...) {
        qDebug() << "Unable to extract features from " << fn;
        return res;
    }

    res.tmp_img = in; //FIXME: debug only
#endif
    res.valid = true;
    match_cache[fn] = res;
    return res;
}

void MViewer::Recognize(const QPixmap &in, QString classifier, bool use_hog)
{
    using namespace cv;

    CascadeClassifier cascade;
    HOGDescriptor hog;
    if (!use_hog && !cascade.load(classifier.toStdString())) {
        qDebug() << "Unable to load cascade from " << classifier;
        return;
    }

    QImage inq = in.toImage();
    std::vector<Rect> items;
    Mat work,inp = quickConvert(inq);
    cvtColor(inp,work,CV_BGR2GRAY);

    try {
        if (!use_hog) {
            equalizeHist(work,work);
            cascade.detectMultiScale(work,items,1.1,3,0,Size(32,32));
        } else {
            hog.setSVMDetector(HOGDescriptor::getDefaultPeopleDetector());
            hog.detectMultiScale(work,items,0,Size(8,8),Size(32,32),1.05,2);
        }
    } catch (...) {
        qDebug() << "Error";
        return;
    }

    qDebug() << items.size() << " items detected";

    QPainter painter(&inq);
    QPen paintpen(Qt::red);
    paintpen.setWidth(2);
    painter.setPen(paintpen);

    for (auto &i : items) {
        painter.drawRect(QRect(i.x,i.y,i.width,i.height));
    }

    ui->label_2->setPixmap(QPixmap::fromImage(inq));
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

void MViewer::on_actionDetect_face_triggered()
{
    if (!current_l.isValid()) return;
    Recognize(current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>(),"/usr/share/opencv/haarcascades/haarcascade_frontalface_alt.xml",false); //OK
}

void MViewer::on_actionDetect_body_triggered()
{
    if (!current_l.isValid()) return;
    Recognize(current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>(),"/usr/share/opencv/haarcascades/haarcascade_upperbody.xml",false);
}

void MViewer::on_actionDetect_face_profile_triggered()
{
    if (!current_l.isValid()) return;
    Recognize(current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>(),"/usr/share/opencv/haarcascades/haarcascade_profileface.xml",false);
}
