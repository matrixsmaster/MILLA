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
            ok = qq.exec("CREATE TABLE stats (file TEXT, views UNSIGNED BIGINT, rating TINYINT, tags TEXT, notes TEXT)");
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
    QSqlDatabase::database().close();
    delete ui;
}

void MViewer::addTag(QString const &tg, bool check)
{
    ui->listWidget->addItem(tg);
    QListWidgetItem* i = ui->listWidget->item(ui->listWidget->count()-1);
    if (!i) return;
    i->setFlags(i->flags() | Qt::ItemIsUserCheckable);
    i->setCheckState(check? Qt::Checked : Qt::Unchecked);
}

void MViewer::updateTags()
{
    QSqlQuery q;
    bool ok = q.exec("SELECT tag FROM tags ORDER BY rating DESC");
    qDebug() << "[db] Read whole tags table: " << ok;
    if (ok) {
        while (q.next()) addTag(q.value(0).toString());
    }
}

void MViewer::on_pushButton_clicked()
{
    if (ui->lineEdit->text().isEmpty()) return;

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
    q.prepare("INSERT INTO tags (key, tag, rating) VALUES (:k, :tg, 1)");
    q.bindValue(":k",key);
    q.bindValue(":tg",ui->lineEdit->text());
    bool ok = q.exec();

    qDebug() << "[db] Inserting tag " << ui->lineEdit->text() << ": " << ok;
    if (ok) addTag(ui->lineEdit->text());
}

void MViewer::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
    if (fileName.isEmpty()) return;
    qDebug() << fileName;

    extra_cache.clear();

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

    bool purelist = false; //TODO: move it to user settings

    ui->listView->setModel(new ThumbnailModel(lst,ui->listView));
    ui->listView->setViewMode(purelist? QListView::ListMode : QListView::IconMode);
    ui->listView->setFlow(purelist? QListView::TopToBottom : QListView::LeftToRight);
    ui->listView->setWrapping(!purelist);
    ui->listView->setSpacing(purelist? 0:10);
    ui->listView->setResizeMode(QListView::Adjust);
    ui->listView->setSelectionMode(QListView::SingleSelection);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
        current_l = ui->listView->selectionModel()->selectedIndexes().first();
        scaleImage(ui->scrollArea,ui->label,&current_l,1);
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

        if (!i->loaded) continue; //FIXME: debug only

        double cur_area = i->picture.size().width() * i->picture.size().height();
        if (orig_area / cur_area > 2 || cur_area / orig_area > 2) continue;

        cur = getExtraCacheLine(i->filename);
        if (!cur.valid) continue;

        double corr = compareHist(orig.hist,cur.hist,CV_COMP_CORREL);
        qDebug() << "Correlation with " << i->filename << ":  " << corr;

        if (corr > 0) targets[corr] = i;
    }
}

MImageExtras MViewer::getExtraCacheLine(QString const &fn)
{
    using namespace cv;

    if (extra_cache.count(fn))
        return extra_cache[fn];

    MImageExtras res;
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
    Mat in = slowConvert(orgm);

    int histSize[] = {64, 64, 64};
    float rranges[] = {0, 256};
    const float* ranges[] = {rranges, rranges, rranges};
    int channels[] = {0, 1, 2};
    calcHist(&in,1,channels,Mat(),res.hist,3,histSize,ranges,true,false);

    extra_cache[fn] = res;
    return res;
}

void MViewer::DetectFaces(const QPixmap &in)
{
    using namespace cv;

    if (!face_cascade) return;

    QImage inq = in.toImage();
    std::vector<Rect> items;
    Mat work,inp = slowConvert(inq);

    try {
        cvtColor(inp,work,CV_BGR2GRAY);
        equalizeHist(work,work);
        face_cascade->detectMultiScale(work,items,1.1,3,0,Size(32,32));
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
    DetectFaces(current_l.data(ThumbnailModel::LargePixmapRole).value<QPixmap>());
}
