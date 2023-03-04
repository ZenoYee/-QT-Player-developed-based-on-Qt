#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QRegExp>
#include <QFileDialog>
#include <QFileInfoList>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_sptrDecoder(new CMainDecoder),
    m_sptrMenuTimer(new QTimer),
    m_sptrProgressTimer(new QTimer)
{
    ui->setupUi(this);

    qRegisterMetaType<CMainDecoder::EnPlayState>("CMainDecoder::EnPlayState");
    m_sptrMenuTimer->start(3000);
    m_sptrProgressTimer->setInterval(500);

    initUi();
    initFFmpeg();
    initSlot();
}

void MainWindow::initUi()
{
    setWindowIcon(QIcon(":/pic/player.png"));
    this->resize(800, 600);
    this->setWindowTitle("Media Player");
    this->centralWidget()->setMouseTracking(true);
    this->setMouseTracking(true);

    ui->labelTime->setStyleSheet("background-color:rgba(0,0,0,0);font-size:12px;font-weight:normal;color:#fff;");
    ui->labelTime->setText(QString("00.00.00 / 00:00:00"));
    ui->sliderVolume->setRange(0, 100);
    ui->sliderVolume->setValue(100);

    ui->sliderProgress->installEventFilter(this);

    QMenu* pMedia = ui->menuBar->addMenu(QStringLiteral("媒体"));
    m_pActOpenFile = pMedia->addAction(QStringLiteral("打开文件"));
    m_pActOpenUrl = pMedia->addAction(QStringLiteral("打开网络串流"));

    ui->cbbSpeed->addItem("0.5x");
    ui->cbbSpeed->addItem("0.75x");
    ui->cbbSpeed->addItem("1.0x");
    ui->cbbSpeed->addItem("1.25x");
    ui->cbbSpeed->addItem("1.5x");
    ui->cbbSpeed->addItem("2.0x");
    ui->cbbSpeed->setCurrentIndex(2);
    ui->cbbSpeed->setItemText(2, QStringLiteral("倍速"));

    ui->btnPlay->setText("");
    ui->btnPlay->setStyleSheet("background-color:rgba(0,0,0,0)");
    ui->btnPlay->setIcon(QIcon(":/pic/play.png"));
    ui->btnPlay->setIconSize(QSize(32,32));

    ui->btnStop->setText("");
    ui->btnStop->setStyleSheet("background-color:rgba(0,0,0,0)");
    ui->btnStop->setIcon(QIcon(":/pic/stop.png"));
    ui->btnStop->setIconSize(QSize(32,32));

    ui->btnVolume->setText("");
    ui->btnVolume->setStyleSheet("background-color:rgba(0,0,0,0)");
    ui->btnVolume->setIcon(QIcon(":/pic/volume.png"));
    ui->btnVolume->setIconSize(QSize(32,32));

    ui->sliderProgress->setStyleSheet("QSlider::groove:horizontal {\
                                      border: 1px solid #4A708B;\
                                      background: #C0C0C0;\
                                      height: 5px;\
                                      border-radius: 1px;\
                                      padding-left:-1px;\
                                      padding-right:-1px;\
                                  }\
                                  \
                                  QSlider::sub-page:horizontal {\
                                      background: qlineargradient(x1:0, y1:0, x2:0, y2:1,\
                                      stop:0 #B1B1B1, stop:1 #c4c4c4);\
                                      background: qlineargradient(x1: 0, y1: 0.2, x2: 1, y2: 1,\
                                      stop: 0 #5DCCFF, stop: 1 #1874CD);\
                                      border: 1px solid #4A708B;\
                                      height: 10px;\
                                      border-radius: 2px;\
                                  }\
                                  \
                                  QSlider::add-page:horizontal {\
                                      background: #575757;\
                                      border: 0px solid #777;\
                                      height: 10px;\
                                      border-radius: 2px;\
                                  }\
                                  QSlider::handle:horizontal\
                                  {\
                                      background: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5,\
                                      stop:0.6 #45ADED, stop:0.778409 rgba(255, 255, 255, 255));\
                                  \
                                      width: 11px;\
                                      margin-top: -3px;\
                                      margin-bottom: -3px;\
                                      border-radius: 5px;\
                                  }\
                                  \
                                  QSlider::handle:horizontal:hover {\
                                      background: qradialgradient(spread:pad, cx:0.5, cy:0.5, radius:0.5, fx:0.5, fy:0.5, stop:0.6 #2A8BDA,\
                                      stop:0.778409 rgba(255, 255, 255, 255));\
                                  \
                                      width: 11px;\
                                      margin-top: -3px;\
                                      margin-bottom: -3px;\
                                      border-radius: 5px;\
                                  }\
                                  \
                                  QSlider::sub-page:horizontal:disabled {\
                                      background: #00009C;\
                                      border-color: #999;\
                                  }\
                                  \
                                  QSlider::add-page:horizontal:disabled {\
                                      background: #eee;\
                                      border-color: #999;\
                                  }\
                                  \
                                  QSlider::handle:horizontal:disabled {\
                                      background: #eee;\
                                      border: 1px solid #aaa;\
                                      border-radius: 4px;\
                                  }\
                                  ");

}

void MainWindow::initFFmpeg()
{
    av_register_all();
    avfilter_register_all();

    if(avformat_network_init() < 0)
    {
        cout << "avformat_network_init failed";
    }
    if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0)
    {
        cout << "SDL_Init failed";
    }
}

void MainWindow::initSlot()
{
    connect(m_sptrDecoder.get(), SIGNAL(sigOpenFileError()), this, SLOT(sltOpenFileError()));
    connect(ui->btnPlay,        SIGNAL(clicked(bool)), this, SLOT(sltButtonClick()));
    connect(ui->btnStop,        SIGNAL(clicked(bool)), this, SLOT(sltButtonClick()));

    connect(m_pActOpenFile, SIGNAL(triggered(bool)), this, SLOT(sltButtonClick()));
    connect(m_pActOpenUrl, SIGNAL(triggered(bool)), this, SLOT(sltButtonClick()));

    connect(m_sptrMenuTimer.get(), SIGNAL(timeout()), this, SLOT(sltTimer()));
    connect(m_sptrProgressTimer.get(), SIGNAL(timeout()), this, SLOT(sltTimer()));
    connect(ui->sliderProgress, SIGNAL(sliderMoved(int)), this, SLOT(sltSeekProgress(int)));
    connect(ui->sliderVolume, SIGNAL(sliderMoved(int)), this, SLOT(sltSetVolume(int)));
    connect(ui->btnVolume, SIGNAL(clicked(bool)), this, SLOT(sltButtonClick()));

    connect(this, SIGNAL(sigSelectedVideoFile(QString, QString)),    m_sptrDecoder.get(), SLOT(sltDecoderFile(QString, QString)));
    connect(this, SIGNAL(sigStopVideo()),                            m_sptrDecoder.get(), SLOT(sltStopVideo()));
    connect(this, SIGNAL(sigPauseVideo()),                           m_sptrDecoder.get(), SLOT(sltPauseVideo()));

    connect(m_sptrDecoder.get(), SIGNAL(sigPlayStateChanged(CMainDecoder::EnPlayState)), this, SLOT(sltPlayStateChanged(CMainDecoder::EnPlayState)));
    connect(m_sptrDecoder.get(), SIGNAL(sigGetVideoTime(qint64)),                        this, SLOT(sltVideoTime(qint64)));
    connect(m_sptrDecoder.get(), SIGNAL(sigGetVideo(QImage)),                            this, SLOT(sltShowVideo(QImage)));

    connect(ui->cbbSpeed, SIGNAL(sigClicked(int)), this, SLOT(sltSetComboBoxText(int)));
    connect(ui->cbbSpeed, SIGNAL(activated(int)), this, SLOT(sltSetComboBoxText(int)));
    connect(ui->cbbSpeed, SIGNAL(activated(int)), this, SLOT(sltSetSpeed(int)));
}

void MainWindow::playVideo(const QString &_strFile)
{
    emit sigStopVideo();
    m_strCurrPlay = _strFile;
    m_strCurrPlayType = getFileType(_strFile);
    if("video" == m_strCurrPlayType)
    {
        m_sptrMenuTimer->start();
    }
    else
    {
        m_sptrMenuTimer->stop();
        if(!m_bMenuIsVisible)
        {
            showCrontrols(true);
            m_bMenuIsVisible = true;
        }
    }
    emit sigSelectedVideoFile(_strFile, m_strCurrPlayType);
}


void MainWindow::setHide(const bool& _bIsLocalFile)
{
    m_vecHide.clear();
    if(_bIsLocalFile)
    {
        ui->labelTime->show();
        ui->sliderProgress->show();
        ui->sliderProgress->show();
        m_vecHide.push_back(ui->labelTime);
        m_vecHide.push_back(ui->sliderProgress);
        m_vecHide.push_back(ui->cbbSpeed);
    }
    else
    {
        ui->labelTime->hide();
        ui->sliderProgress->hide();
        ui->cbbSpeed->hide();
    }

    m_vecHide.push_back(ui->btnPlay);
    m_vecHide.push_back(ui->btnStop);
    m_vecHide.push_back(ui->sliderVolume);
    m_vecHide.push_back(ui->btnVolume);
}

void MainWindow::showCrontrols(const bool &_bShow)
{
    if(_bShow)
    {
        QApplication::setOverrideCursor(Qt::ArrowCursor);
        for(auto it_pWgt : m_vecHide)
            it_pWgt->show();
    }
    else
    {
        QApplication::setOverrideCursor(Qt::BlankCursor);
        for(auto it_pWgt : m_vecHide)
            it_pWgt->hide();
    }

}

QString MainWindow::getFileType(const QString &_strFile)
{
    QString strSuffix = _strFile.right(_strFile.size() - _strFile.lastIndexOf(".") - 1);
    if("mp3" == strSuffix || "aac" == strSuffix || "ape" == strSuffix || "flac" == strSuffix || "wav" == strSuffix)
        return "music";
    else
        return "video";
}

QString MainWindow::getFileName(const QString& _strPath)
{
    return _strPath.right(_strPath.size() - _strPath.lastIndexOf("/") - 1);
}

void MainWindow::paintEvent(QPaintEvent *_pEvent)
{
    Q_UNUSED(_pEvent);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int nWidth = this->width();
    const int nHeight = this->height();

    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, nWidth, nHeight);

    if(m_ImgVideo.isNull())
        return;
    if(m_bIsKeepAspectRation)
    {
        QImage img = m_ImgVideo.scaled(QSize(nWidth, nHeight), Qt::KeepAspectRatio);

        int x = (this->width() - img.width()) / 2;
        int y = (this->height() - img.height()) / 2;
        painter.drawImage(QPoint(x, y), img);
    }
    else
    {
        QImage img = m_ImgVideo.scaled(QSize(nWidth, nHeight));
        painter.drawImage(QPoint(0, 0), img);
    }
}

void MainWindow::closeEvent(QCloseEvent *_pEvent)
{
    emit sigStopVideo();
}

void MainWindow::changeEvent(QEvent *_pEvent)
{
    if(QEvent::WindowStateChange == _pEvent->type())
    {
        if(Qt::WindowMinimized == this->windowState())
        {
            _pEvent->ignore();
            this->hide();
        }
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *_pEvent)
{
    Q_UNUSED(_pEvent);
    m_sptrMenuTimer->stop();
    if(!m_bMenuIsVisible)
    {
        showCrontrols(true);
        m_bMenuIsVisible = true;
        QApplication::setOverrideCursor(Qt::ArrowCursor);
    }
    m_sptrMenuTimer->start();

    if(m_bDrag)
    {
        QPoint distancePoint = _pEvent->globalPos() - m_mouseStartPoint;
        this->move(m_winTopLeftPoint + distancePoint);
    }
}

void MainWindow::mousePressEvent(QMouseEvent *_pEvent)
{
    if(_pEvent->button() == Qt::LeftButton)
    {
        m_bDrag = true;
        m_mouseStartPoint = _pEvent->globalPos();
        m_winTopLeftPoint = this->frameGeometry().topLeft();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *_pEvent)
{
    if(_pEvent->button() == Qt::LeftButton)
    {
        m_bDrag = false;   
    }
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *_pEvent)
{
    if(_pEvent->button() == Qt::LeftButton)
    {
        if(this->isFullScreen())
        {
            this->showNormal();
        }
        else
        {
            this->showFullScreen();
        }
    }
}


void MainWindow::sltButtonClick()
{
    QString strFilePath;

    if(QObject::sender() == m_pActOpenFile)
    {
        setHide();
        strFilePath = QFileDialog::getOpenFileName(this, "chose file to play", "D:\\Code\\C++\\Qt",
                                                   "(*.264 *.mp4 *.rmvb *.avi *.mov *.flv *.mkv *.ts *.mp3 *.flac *.ape *.wav)");
        if(!strFilePath.isNull() || !strFilePath.isEmpty())
        {
            playVideo(strFilePath);
        }
    }
    else if(QObject::sender() == m_pActOpenUrl)
    {
        setHide(m_bIsUrl);

        bool bRet = false;
        strFilePath = QInputDialog::getText(this, QStringLiteral("输入网络串流"),QStringLiteral("网络串流地址"),QLineEdit::Normal, "rtmp://mobliestream.c3tv.com:554/live/goodtv.sdp", &bRet, Qt::WindowCloseButtonHint);

        if(!strFilePath.isNull() || !strFilePath.isEmpty() || bRet)
        {
            emit sigStopVideo();
            emit sigSelectedVideoFile(strFilePath, "video");
        }
    }
    else if(QObject::sender() == ui->btnPlay)
    {
        emit sigPauseVideo();
    }
    else if(QObject::sender() == ui->btnStop)
    {
        emit sigStopVideo();
    }
    else if(QObject::sender() == ui->btnVolume)
    {
        static bool bMute = false;
        if(!bMute)
        {
            ui->btnVolume->setIcon(QIcon(":/pic/volume-mute.png"));
            ui->sliderVolume->setValue(0);
            m_sptrDecoder->SetVolume(0);
            bMute = true;
        }
        else
        {
            ui->btnVolume->setIcon(QIcon(":/pic/volume.png"));
            ui->sliderVolume->setValue(100);
            m_sptrDecoder->SetVolume(100);
            bMute = false;
        }
    }

}

void MainWindow::sltTimer()
{
    if(QObject::sender() == m_sptrMenuTimer.get())
    {
        if(CMainDecoder::EN_PLAYING == m_enPlayState && m_bMenuIsVisible)
        {
            if(isFullScreen())
            {
                QApplication::setOverrideCursor(Qt::BlankCursor);
            }
            showCrontrols(false);
            m_bMenuIsVisible = false;
        }
    }
    else if(QObject::sender() == m_sptrProgressTimer.get())
    {
        if(CMainDecoder::EN_PAUSE == m_enPlayState && m_bMenuIsVisible)
        {
            return;
        }

        int64_t nCurrTime = static_cast<int64_t>(m_sptrDecoder->GetCurrentTime());
        ui->sliderProgress->setValue(static_cast<int32_t>(nCurrTime));

        int nCurHour = nCurrTime / 60 / 60;
        int nCurMin = (nCurrTime / 60) % 60;
        int nCurSec = nCurrTime % 60;

        int nTotalHour = m_nTotalTime / 60 / 60;
        int nTotalMin = (m_nTotalTime / 60) % 60;
        int nTotalSec = m_nTotalTime % 60;

        ui->labelTime->setText(QString("%1:%2:%3 / %4:%5:%6")
                               .arg(nCurHour, 2, 10, QLatin1Char('0'))
                               .arg(nCurMin, 2, 10, QLatin1Char('0'))
                               .arg(nCurSec, 2, 10, QLatin1Char('0'))
                               .arg(nTotalHour, 2, 10, QLatin1Char('0'))
                               .arg(nTotalMin, 2, 10, QLatin1Char('0'))
                               .arg(nTotalSec, 2, 10, QLatin1Char('0'))
                               );

    }
}

void MainWindow::sltEditText()
{
    m_sptrMenuTimer->stop();
    m_sptrMenuTimer->start();
}

void MainWindow::sltSeekProgress(int _nValue)
{
    m_sptrDecoder->SeekProgress(static_cast<uint64_t>(_nValue) * 1000000);
}

void MainWindow::sltVideoTime(qint64 _nTime)
{
    m_nTotalTime = _nTime / 1000000;
    ui->sliderProgress->setRange(0, m_nTotalTime);

    int32_t nH = m_nTotalTime / 60 / 60;
    int32_t nM = (m_nTotalTime / 60) % 60;
    int32_t nS = m_nTotalTime % 60;

    ui->labelTime->setText(QString("00:00:00 / %1:%2:%3")
                           .arg(nH, 2, 10, QLatin1Char('0'))
                           .arg(nM, 2, 10, QLatin1Char('0'))
                           .arg(nS, 2, 10, QLatin1Char('0'))
                           );
}

void MainWindow::sltPlayStateChanged(CMainDecoder::EnPlayState _enState)
{
    switch (_enState)
    {
        case CMainDecoder::EN_PLAYING:
        {
            ui->btnPlay->setIcon(QIcon(":/pic/pause.png"));
            m_enPlayState = CMainDecoder::EN_PLAYING;
            m_sptrProgressTimer->start();
        }
        break;

        case CMainDecoder::EN_PAUSE:
        {
            ui->btnPlay->setIcon(QIcon(":/pic/play.png"));
            m_enPlayState = CMainDecoder::EN_PAUSE;
        }
        break;

        case CMainDecoder::EN_STOP:
        {
            m_ImgVideo = QImage(":/pic/stop_background.jpg");
            ui->btnPlay->setIcon(QIcon(":/pic/play.png"));
            m_enPlayState = CMainDecoder::EN_STOP;

            m_sptrProgressTimer->stop();
            ui->labelTime->setText("00:00:00 / 00:00:00");
            ui->sliderProgress->setValue(0);
            m_nTotalTime = 0;
            update();
        }
        break;

        case CMainDecoder::EN_FINISH:
        {
            m_ImgVideo = QImage(":/pic/play.png");
            m_enPlayState = CMainDecoder::EN_STOP;

            m_sptrProgressTimer->stop();
            ui->labelTime->setText("00:00:00 / 00:00:00");
            ui->sliderProgress->setValue(0);
            m_nTotalTime = 0;
        }
        break;

    default:
        break;
    }
}

void MainWindow::sltShowVideo(QImage _img)
{
    m_ImgVideo = _img;
    update();
}

void MainWindow::sltSetVolume(int _nValue)
{
    showCrontrols(true);
    m_sptrDecoder->SetVolume(_nValue);
    if(0 == _nValue)
    {
        ui->btnVolume->setIcon(QIcon(":/pic/volume-mute.png"));
    }
    else  if(_nValue > 0)
    {
        ui->btnVolume->setIcon(QIcon(":/pic/volume.png"));
    }
}

void MainWindow::sltSetComboBoxText(int _nValue)
{
   ui->cbbSpeed->setItemText(2, QStringLiteral("1.0x"));
   if(2 == _nValue)
       ui->cbbSpeed->setItemText(2, QStringLiteral("倍速"));
}

void MainWindow::sltSetSpeed(int _nValue)
{
    switch(_nValue)
    {
    case 0:
         m_sptrDecoder->SetSpeed(0.5);
        break;
    case 1:
         m_sptrDecoder->SetSpeed(0.75);
        break;
    case 2:
        m_sptrDecoder->SetSpeed(1);
        break;
    case 3:
        m_sptrDecoder->SetSpeed(1.25);
        break;
    case 4:
        m_sptrDecoder->SetSpeed(1.5);
        break;
    case 5:
        m_sptrDecoder->SetSpeed(2);
        break;
    default:
        break;
    }

}

void MainWindow::sltOpenFileError()
{
    QMessageBox::information(nullptr, "ERROR", "Open media file failed,please check file");
}


MainWindow::~MainWindow()
{
    delete ui;
}
