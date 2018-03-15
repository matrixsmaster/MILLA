#include "mviewer.h"
#include "ui_mviewer.h"

MViewer::MViewer(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MViewer)
{
    ui->setupUi(this);
}

MViewer::~MViewer()
{
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

    QObject::connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged, [this] {
        QModelIndex selectedIndex = ui->listView->selectionModel()->selectedIndexes().first();
        // We use our custom role here to retrieve the large image using the selected index.
        ui->label->setPixmap(selectedIndex.data(ThumbnailModel::LargePixmapRole).value<QPixmap>());
        //ui->label->setText(selectedIndex.data(ThumbnailModel::FullPathRole).value<QString>());
        ui->label->adjustSize();
        //ui->listView->adjustSize();
    });
}
