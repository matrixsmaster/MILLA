#include "starlabel.h"

StarLabel::StarLabel(QWidget* parent, Qt::WindowFlags /*f*/)
    : QLabel(parent)
{
}

StarLabel::~StarLabel()
{
}

void StarLabel::mousePressEvent(QMouseEvent* /*event*/)
{
    emit clicked();
}

void StarLabel::setStarActivated(bool a)
{
    setPixmap(QPixmap(a? ":/star-filled.png" : ":/star-empty.png"));
    _activated = a;
}
