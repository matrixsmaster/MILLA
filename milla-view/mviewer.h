#ifndef MVIEWER_H
#define MVIEWER_H

#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QDirIterator>
#include <QFileInfo>
#include <QScrollArea>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>
#include <QPainter>
#include <QMovie>
#include <QCryptographicHash>
#include <QInputDialog>
#include <ctime>
#include <chrono>
#include "db_format.h"
#include "plugins.h"
#include "pluginloader.h"
#include "cvhelper.h"
#include "thumbnailmodel.h"
#include "sresultmodel.h"
#include "exportform.h"
#include "mimpexpmodule.h"
#include "facedetector.h"

#define MILLA_VERSION "ver. 0.1.8"
#define MILLA_SITE "http://github.com/matrixsmaster/MILLA"
#define EXTRA_CACHE_SIZE 1500
#define MILLA_OPEN_FILE "Image Files (*.png *.jpg *.jpeg *.bmp)"
#define MILLA_OPEN_LIST "Text Files [txt,lst] (*.txt *.lst)"

/* A note for future self:
 * This file SHOULD be separated. It IS in this messy state only because MILLA is
 * mostly a research software.
 * But as soon, as some functionality seems to be complete, it should be
 * extracted, encapsulated and moved into its own module.
 * Just like ImportExportModule and FaceDetector did.
 *
 * Thank you in advance, future me!
 * */

enum MROIType {
    MROI_GENERIC = 0,
    MROI_FACE_FRONTAL,
    MROI_INVALID // terminator
};

struct MROI {
    MROIType kind = MROI_GENERIC;
    int x = 0, y = 0, w = 0, h = 0;
};

struct MImageExtras {
    bool valid = false;
    QSize picsize;
    std::vector<MROI> rois;
    cv::Mat hist;
    bool color = true;
    QByteArray sha;
    qint64 filelen;
};

struct MHistory {
    QStringList files;
    QStringList::iterator cur;
};

namespace Ui {
class MViewer;
}

class MViewer : public QMainWindow
{
    Q_OBJECT

public:
    explicit MViewer(QWidget *parent = 0);
    ~MViewer();

    void processArguments();

private slots:
    void on_pushButton_clicked();

    void on_actionOpen_triggered();

    void on_actionFit_triggered();

    void on_actionMatch_triggered();

    void on_listWidget_itemClicked(QListWidgetItem *item);

    void on_actionLoad_everything_slow_triggered();

    void on_radio_search_toggled(bool checked);

    void on_radio_settags_toggled(bool checked);

    void on_actionJump_to_triggered();

    void on_actionRefine_search_triggered();

    void on_actionSwap_images_triggered();

    void on_actionClear_results_triggered();

    void on_actionQuit_triggered();

    void on_actionLink_left_to_right_triggered();

    void on_actionOpen_list_triggered();

    void on_actionAbout_triggered();

    void on_pushButton_2_clicked();

    void on_actionKudos_to_left_image_triggered();

    void on_actionKudos_to_right_image_triggered();

    void on_actionLink_bidirectional_triggered();

    void on_actionLeft_image_triggered();

    void on_actionRight_image_triggered();

    void on_actionClear_image_cache_triggered();

    void on_actionThumbnails_cloud_changed();

    void on_actionExport_data_triggered();

    void on_actionImport_data_triggered();

    void on_actionUpdate_thumbnails_triggered();

    void on_action_None_triggered();

    void on_actionBy_name_triggered();

    void on_actionBy_time_triggered();

    void on_actionDescending_triggered();

    void on_actionList_all_triggered();

    void on_actionReload_metadata_triggered();

    void on_actionSanitize_DB_triggered();

    void on_actionPrevious_triggered();

    void on_actionNext_triggered();

    void on_actionPrevious_2_triggered();

    void on_actionNext_2_triggered();

private:
    Ui::MViewer *ui;
    MillaPluginLoader plugins;

    QTimer view_timer;
    QProgressBar* progressBar;
    QPushButton* stopButton;
    QMovie* loadingMovie;
    QLabel* loadingLabel = nullptr;

    bool flag_stop_load_everything = false;
    double scaleFactor = 1;
    MImageListRecord current_l, current_r;

    FaceDetector facedetector;
    std::map<QString,MImageExtras> extra_cache;
    MTagCache tags_cache;
    MHistory history;

    bool initDatabase();

    void cleanUp();

    void checkExtraCache();

    void showImageList(QStringList const &lst);

    bool isLoadableFile(QString const &path, QString* canonicalPath);

    void scanDirectory(QString const &dir, QStringList &addto, bool recursive);

    void openDirByFile(QString const &fileName, bool recursive = false);

    void openDirByList(QString const &fileName);

    void leftImageMetaUpdate();

    void showSelectedImage();

    void addTag(QString const &tg, int key, bool check = false);

    void updateTags(QString fn = QString());

    void updateStars(QString fn = QString());

    void changedStars(int n);

    void prepareLongProcessing(bool finish = false);

    QByteArray getSHA256(QString const &fn, qint64 *size);

    bool createStatRecord(QString const &fn, bool cache_global = false);

    bool insertStatRecord(QString const &fn, MImageExtras &rec, bool update);

    unsigned incViews(bool left = true);

    void scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, double factor);

    QString timePrinter(double sec) const;

    MImageExtras getExtrasFromDB(QString const &fn);

    MImageExtras collectImageExtraData(QString const &fn, QPixmap const &org);

    MImageExtras getExtraCacheLine(QString const &fn, bool forceload = false, bool ignore_thumbs = false);

    void updateCurrentTags();

    void resultsPresentation(QStringList lst, QListView *view, int tabIndex);

    void searchResults(QStringList lst);

    void searchByTag();

    void displayLinkedImages(QString const &fn);

    void kudos(MImageListRecord const &to, int delta);

    void selectIEFileDialog(bool import);

    void updateThumbnailsOrder(ThumbnailModel::ThumbnailModelSort ord, bool desc);

    void sanitizeLinks();
};

#endif // MVIEWER_H
