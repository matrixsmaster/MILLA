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
    //QMessageBox::question(this, "Save", "Do you want to save your changes?", 0);
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

//    ui->label->setScaledContents(true);
//    ui->label_2->setScaledContents(true);

    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
        QModelIndex selectedIndex = ui->listView->selectionModel()->selectedIndexes().first();
        // We use our custom role here to retrieve the large image using the selected index.
        //ui->label->setPixmap(selectedIndex.data(ThumbnailModel::LargePixmapRole).value<QPixmap>());
//        scaleFactor = 1;
//        scaleImage(1);
        //ui->label->adjustSize();
        //ui->label->resize(ui->scrollArea->size());
        qDebug() << ui->scrollArea->size();
        current = selectedIndex;
        scaleImage(1);
        qDebug() << "selChanged";
    });
    connect(ui->listView, &QListView::customContextMenuRequested, this, [this] {
        QModelIndex selectedIndex = ui->listView->selectionModel()->selectedIndexes().first();
        ui->label_2->setPixmap(selectedIndex.data(ThumbnailModel::LargePixmapRole).value<QPixmap>());
        //ui->label_2->adjustSize();
        qDebug() << "rightClick";
    });
}

#if 0
void MViewer::scaleImage(double factor)
{
    Q_ASSERT(ui->label->pixmap());
    scaleFactor *= factor;
    ui->label->resize(scaleFactor * ui->label->pixmap()->size());

    adjustScrollBar(ui->scrollArea->horizontalScrollBar(), factor);
    adjustScrollBar(ui->scrollArea->verticalScrollBar(), factor);

    //zoomInAct->setEnabled(scaleFactor < 3.0);
    //zoomOutAct->setEnabled(scaleFactor > 0.333);
}

void MViewer::adjustScrollBar(QScrollBar *scrollBar, double factor)
{
    scrollBar->setValue(int(factor * scrollBar->value() + ((factor - 1) * scrollBar->pageStep()/2)));
}

void MViewer::normalSize()
{
    ui->label->adjustSize();
    scaleFactor = 1.0;
}
#else

void MViewer::scaleImage(double factor)
{
    if (!current.isValid()) return;

    scaleFactor *= factor;
    factor = scaleFactor;
    qDebug() << "factor = " << factor;

    if (ui->actionFit->isChecked()) {
        ui->scrollArea->setWidgetResizable(false);
        ui->label->setPixmap(QPixmap());
        ui->label->setText("");
        ui->label->updateGeometry();
        ui->scrollArea->updateGeometry();
        ui->scrollArea->setWidgetResizable(true);
        ui->label->setPixmap(current.data(ThumbnailModel::LargePixmapRole).value<QPixmap>().scaled(ui->label->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation));
    }
}

#endif

void MViewer::on_actionFit_triggered()
{
    bool fitToWindow = ui->actionFit->isChecked();
    //ui->scrollArea->setWidgetResizable(fitToWindow);
    //if (!fitToWindow) normalSize();
}

void MViewer::on_actionReisable_triggered()
{
    ui->scrollArea->setWidgetResizable(ui->actionReisable->isChecked());
    qDebug() << "resizeable: " << ui->actionReisable->isChecked();
}
