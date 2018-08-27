#include "storyselector.h"
#include "ui_storyselector.h"
#include "dbhelper.h"

StorySelector::StorySelector(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StorySelector)
{
    ui->setupUi(this);

    QStringList lst = DBHelper::getStoriesList();
    for (auto &i : lst) ui->listWidget->addItem(i);
}

StorySelector::~StorySelector()
{
    delete ui;
}

QString StorySelector::getStoryTitle()
{
    if (ui->listWidget->currentItem())
        return ui->listWidget->currentItem()->text();
    else
        return QString();
}
