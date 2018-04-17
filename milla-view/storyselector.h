#ifndef STORYSELECTOR_H
#define STORYSELECTOR_H

#include <QDialog>

namespace Ui {
class StorySelector;
}

class StorySelector : public QDialog
{
    Q_OBJECT

public:
    explicit StorySelector(QWidget *parent = 0);
    ~StorySelector();

    QString getStoryTitle();

private:
    Ui::StorySelector *ui;
};

#endif // STORYSELECTOR_H
