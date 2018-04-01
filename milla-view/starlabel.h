#ifndef STARLABEL_H
#define STARLABEL_H

#include <QLabel>
#include <QWidget>
#include <Qt>
#include <QPixmap>

class StarLabel : public QLabel {
    Q_OBJECT

public:
    explicit StarLabel(QWidget* parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags());
    ~StarLabel();

    void setStarActivated(bool a);
    bool isStarActivated()          { return _activated; }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event);

private:
    bool _activated = true;
};

#endif // STARLABEL_H
