#ifndef MVIEWER_H
#define MVIEWER_H

#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QScrollBar>
#include <QScrollArea>
#include <QListWidget>
#include <QProgressBar>
#include <QTimer>
#include <QPainter>
#include <QMovie>
#include <QInputDialog>
#include <QMetaMethod>
#include <ctime>
#include <cstdlib>
#include <chrono>
#include "shared.h"
#include "pluginloader.h"
#include "dbhelper.h"
#include "mimageloader.h"
#include "thumbnailmodel.h"
#include "sresultmodel.h"
#include "mmemorymodel.h"
#include "exportform.h"
#include "mimpexpmodule.h"
#include "mmatcher.h"
#include "mimageops.h"
#include "storyselector.h"
#include "listeditor.h"

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

    void prepareLongProcessing(bool finish = false);

    void showGeneratedPicture(QPixmap const &in);

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

    void on_actionRandom_image_triggered();

    void on_actionClose_triggered();

    void on_actionFind_duplicates_triggered();

    void on_actionRepeat_last_triggered();

    void on_actionAlways_show_GUI_toggled(bool arg1);

    void on_actionHotkeys_enabled_toggled(bool arg1);

    void on_actionClear_triggered();

    void on_actionRotate_90_CW_triggered();

    void on_actionRotate_90_CCW_triggered();

    void on_actionFlip_vertical_triggered();

    void on_actionFlip_horizontal_triggered();

    void on_actionZoom_in_triggered();

    void on_actionZoom_out_triggered();

    void on_actionReset_zoom_triggered();

    void on_actionClear_all_triggered();

    void on_lineEdit_2_textChanged(const QString &arg1);

    void on_actionApply_tagset_triggered();

    void on_actionApply_tagset_from_left_to_right_triggered();

    void on_actionConcatenate_triggered();

    void on_pushButton_9_clicked();

    void on_pushButton_4_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_7_clicked();

    void on_actionPick_a_story_triggered();

    void on_actionAdd_to_story_triggered();

    void on_actionCrop_triggered();

    void on_actionInfo_triggered();

    void on_actionOpen_with_triggered();

    void on_actionEdit_exclusion_list_triggered();

private:
    Ui::MViewer *ui;
    DBHelper db;
    CVHelper mCV;
    MillaPluginLoader plugins;
    MImageLoader* loader = nullptr;

    QTimer view_timer;
    QProgressBar* progressBar;
    QPushButton* stopButton;
    QMovie* loadingMovie;
    QLabel* loadingLabel = nullptr;
    ProgressCB thumbload_pbar;

    bool stop_flag = false;
    double scaleFactor = 1;
    MImageListRecord current_l, current_r;
    QRect selection;
    int selection_fsm = 0;
    QPixmap selection_bak;
    SearchFormData last_search;
    int search_cnt = 0;

    std::map<QString,MImageExtras> extra_cache;
    MTagCache tags_cache;
    MHistory history;
    std::map<QAction*,QKeySequence> hotkeys;
    MImageOps* a_story = nullptr;

    void cleanUp();

    void addTag(QString const &tg, unsigned key, bool check = false);

    void updateTags(QString const &fn = QString());

    void updateFileTags();

    void updateStars(QString const &fn = QString());

    void changedStars(int n);

    bool createStatRecord(QString const &fn, bool cache_global = false);

    void checkExtraCache();

    MImageExtras getExtraCacheLine(QString const &fn, bool forceload = false, bool ignore_thumbs = false);

    void showImageList(QStringList const &lst);

    void showSelectedImage();

    void scaleImage(const MImageListRecord &rec, QScrollArea* scrl, QLabel* lbl, QLabel* inflbl, double factor);

    unsigned incViews(bool left = true);

    void leftImageMetaUpdate();

    void resultsPresentation(QStringList lst, QListView *view, int tabIndex);

    void searchResults(QStringList lst);

    void displayLinkedImages(QString const &fn);

    void kudos(MImageListRecord const &to, int delta);

    void selectIEFileDialog(bool import);

    void updateThumbnailsOrder(ThumbnailModel::ThumbnailModelSort ord, bool desc);

    void historyShowCurrent();

    void enableShortcuts(QObjectList const &children, bool en);

    void enableMouseMoveEvents(QObjectList const &lst);

    void updateRecents();

    void copyTagsetTo(QString const &fn);

    void updateStory(QPixmap const &result);

    bool eventFilter(QObject *obj, QEvent *event);

    void printInfo(QString title, MImageListRecord const &targ);
};

#endif // MVIEWER_H
