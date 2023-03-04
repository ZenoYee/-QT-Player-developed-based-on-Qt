#ifndef CSLIDER_H
#define CSLIDER_H

#include <QObject>
#include <QSlider>
#include <QMouseEvent>
class CSlider : public QSlider
{
public:
    CSlider(QWidget* _parent);
    ~CSlider() = default;

    void mousePressEvent(QMouseEvent * _pEvent);

};

#endif // CSLIDER_H
