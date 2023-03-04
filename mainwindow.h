#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QTimer>
#include <QVector>
#include <QList>
#include <QSystemTrayIcon>
#include <QEvent>
#include <QPainter>
#include <QCloseEvent>
#include <QAction>
#include "maindecoder.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow();

private:
    void initUi();

    void initFFmpeg();

    void initSlot();

    void playVideo(const QString& _strFile);

    void setHide(const bool& _bIsLocalFile=true);

    void showCrontrols(const bool& _bShow);

    QString getFileType(const QString& _strFile);

    inline QString getFileName(const QString& _strPath);

private:
    void paintEvent(QPaintEvent* _pEvent);

    void closeEvent(QCloseEvent* _pEvent);

    void changeEvent(QEvent* _pEvent);

    void mouseMoveEvent(QMouseEvent* _pEvent);

    void mousePressEvent(QMouseEvent* _pEvent);

    void mouseReleaseEvent(QMouseEvent* _pEvent);

    void mouseDoubleClickEvent(QMouseEvent* _pEvent);

public slots:
    void sltButtonClick();

    void sltTimer();

    void sltEditText();

    void sltSeekProgress(int _nValue);

    void sltVideoTime(qint64 _nTime);

    void sltPlayStateChanged(CMainDecoder::EnPlayState _enState);

    void sltShowVideo(QImage _img);

    void sltSetVolume(int _nValue);

    void sltSetComboBoxText(int _nValue);

    void sltSetSpeed(int _nValue);

    void sltOpenFileError();

signals:
    void sigSelectedVideoFile(QString _strFile, QString _strType);

    void sigStopVideo();

    void sigPauseVideo();
private:
    Ui::MainWindow *ui = {nullptr};

    bool m_bDrag = {false};
    QPoint m_mouseStartPoint;
    QPoint m_winTopLeftPoint;

    shared_ptr<CMainDecoder> m_sptrDecoder;

    QVector<QWidget*> m_vecHide;

    QString m_strCurrPlay;
    QString m_strCurrPlayType;

    shared_ptr<QTimer> m_sptrMenuTimer;
    shared_ptr<QTimer> m_sptrProgressTimer;


    bool m_bMenuIsVisible = {true};
    bool m_bIsKeepAspectRation = {false};

    QImage m_ImgVideo;
    const bool m_bIsUrl = {false};

    CMainDecoder::EnPlayState m_enPlayState = {CMainDecoder::EN_STOP};

    int64_t m_nTotalTime = {0};
    int32_t m_nSeekInterval = {15};

    QAction * m_pActOpenFile = {nullptr};
    QAction * m_pActOpenUrl = {nullptr};
};

#endif // MAINWINDOW_H
