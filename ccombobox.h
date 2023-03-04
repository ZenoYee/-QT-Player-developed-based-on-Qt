#ifndef CCOMBOBOX_H
#define CCOMBOBOX_H

#include <QObject>
#include <QComboBox>
#include <QMouseEvent>
class CComboBox : public QComboBox
{
    Q_OBJECT
public:
    CComboBox(QWidget* _parent);

protected:
    void mousePressEvent(QMouseEvent*) override;

    void mouseReleaseEvent(QMouseEvent*) override;

    void showPopup() override;

signals:
    void sigClicked(int);

};

#endif // CCOMBOBOX_H
