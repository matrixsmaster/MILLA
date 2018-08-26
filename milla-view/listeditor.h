#ifndef LISTEDITOR_H
#define LISTEDITOR_H

#include <QDialog>

namespace Ui {
class ListEditor;
}

class ListEditor : public QDialog
{
    Q_OBJECT

public:
    explicit ListEditor(QWidget *parent = 0);
    ~ListEditor();

    void setTextLabel(QString const &s);

    void setList(QStringList const &lst);

    QStringList getList();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

private:
    Ui::ListEditor *ui;
};

#endif // LISTEDITOR_H
