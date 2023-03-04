#include "ccombobox.h"

CComboBox::CComboBox(QWidget* _parent):QComboBox (_parent)
{

}

void CComboBox::mousePressEvent(QMouseEvent *_pEv)
{
    if(Qt::LeftButton == _pEv->button())
    {
        emit sigClicked(0);
    }
    QComboBox::mousePressEvent(_pEv);
}

void CComboBox::mouseReleaseEvent(QMouseEvent *_pEv)
{
    if(Qt::LeftButton == _pEv->button())
    {
        emit sigClicked(1);
    }

}

void CComboBox::showPopup()
{
    QComboBox::showPopup();
    QWidget *popup = this->findChild<QFrame*>();
    popup->move(popup->x(),popup->y()-this->height()-popup->height());//x轴不变，y轴向上移动 list的高+combox的高
}
