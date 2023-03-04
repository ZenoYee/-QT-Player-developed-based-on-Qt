#ifndef MAINDECODER_H
#define MAINDECODER_H
#include <QThread>
#include <QImage>

extern "C"
{
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libswscale/swscale.h"
#include "libavdevice/avdevice.h"
#include "libavutil/pixfmt.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libavutil/imgutils.h"
}
#include<memory>
#include "audiodecoder.h"
using std::shared_ptr;
using std::make_shared;
class CMainDecoder:public QThread
{
    Q_OBJECT
public:
    enum EnPlayState
    {
        EN_STOP,
        EN_PAUSE,
        EN_PLAYING,
        EN_FINISH
    };
    explicit CMainDecoder();

    ~CMainDecoder();

    double GetCurrentTime() const;

    void SeekProgress(const uint64_t& _nPos);

    int GetVolume() const;

    void SetVolume(const int& _nVolume);

    void SetSpeed(const float& _fValue);

private:
    int initFilter();

    void run();

    static int videoThread(void* _pointer);

    void clearData();

    void setPlayStat(const EnPlayState& _enState);

    void displayVideo(QImage _img);

    double synchonize(AVFrame* _pFrame, double _dPts);

    bool isRealTime(AVFormatContext* _pFmtCtx);

public slots:
    void sltDecoderFile(QString _strFile, QString _strType);

    void sltStopVideo();

    void sltPauseVideo();

    void sltFinishedAudio();


signals:
    void sigReadFinished();

    void sigGetVideo(QImage _img);

    void sigGetVideoTime(qint64 _nTime);

    void sigPlayStateChanged(CMainDecoder::EnPlayState _enState);

    void sigOpenFileError();

private:
    int32_t m_nFileType = {0};
    int32_t m_nVideoIndex = {0};
    int32_t m_nAudioIndex = {0};
    int32_t m_nSubtitileIndex = {0};

    QString m_strCurrFile;
    QString m_strCurrType;

    int64_t m_nTotalTime = {0};
    double m_dVideoClk = {0};

    AVPacket m_seekPkt;
    int64_t m_nSeekPos = {0};
    double m_dSeekTime = {0};

    EnPlayState m_enPlayState = {EN_STOP};
    bool m_bIsStop = {false};
    bool m_bGotStop = {false};
    bool m_bIsPause = {false};
    bool m_bIsSeek = {false};
    bool m_bIsReadFinished = {false};
    bool m_bIsDecodeFinished = {false};

    AVFormatContext* m_pFmtCtx = {nullptr};
    AVCodecContext* m_pCodecCtx = {nullptr};

    CAvPacketQueue m_queVideo;
    CAvPacketQueue m_queSubtitle;
    AVStream* m_pVideoStream = {nullptr};

    shared_ptr<CAudioDecoder> m_sptrAudioDecoder;

    AVFilterGraph* m_pFltGph = {nullptr};
    AVFilterContext* m_pSinkFilCtx = {nullptr};
    AVFilterContext* m_pSrcFilCtx = {nullptr};

};

#endif // MAINDECODER_H
