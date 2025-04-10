#ifndef PLUGINDOCK_H
#define PLUGINDOCK_H

#include <QDialog>

// #ifndef LIBCODE_TEST
// #  define MYAPP_EXPORT Q_DECL_EXPORT
// #else
// #  define MYAPP_EXPORT Q_DECL_IMPORT
// #endif

namespace Ui {
class PluginDock;
}

class PluginDock : public QDialog
{
    Q_OBJECT

public:
    explicit PluginDock(QWidget *parent = nullptr, QWidget* child = nullptr);
    ~PluginDock();

private:
    Ui::PluginDock *ui;
};

#endif // PLUGINDOCK_H
