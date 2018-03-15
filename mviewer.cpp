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
    if (!idx->isValid()) return;

    scaleFactor *= factor;
    scrl->setWidgetResizable(false);
    lbl->setPixmap(QPixmap());
    lbl->setText("");
    lbl->updateGeometry();
    scrl->updateGeometry();
    scrl->setWidgetResizable(true);

    reinterpret_cast<ThumbnailModel*>(ui->listView->model())->LoadUp(idx->row());
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
