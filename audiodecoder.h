#ifndef AUDIODECODER_H
#define AUDIODECODER_H

#include <QObject>
#include "avpacketqueue.h"
extern "C"
{
    #include "libswresample/swresample.h"
}
#include <QDebug>
#define cout qDebug() << __FILE__ << " " << __LINE__ << " "
class CAudioDecoder:public QObject
{
    Q_OBJECT
public:
    explicit CAudioDecoder(QObject* _pObj = nullptr);

    ~CAudioDecoder();

    int OpenAudio(AVFormatContext* _pFmtCtx,  const int32_t& _index);

    void CloseAudio();

    void PauseAudio();

    void StopAudio();

    int GetVolume() const;

    void SetVolume(const int& _nVolume);

    void SetSpeed(const float& _fValue);

    double GetAudioClock();

    void PacketEnqueue(const AVPacket& _pkt);

    void ClearAudioData();

private:
    int decodeAudio();

    void setSampleFormat(const uint32_t& _audioDecFmt);

    static void audioCallBack(void* _pUserData, uint8_t* _pStream, int _SDL_AudioBuffSize);

signals:
    void sigPlayFinished();

public slots:
    void sltReadFileFinished();

private:
    bool m_bIsStop = {false};
    bool m_bIsPause = {false};
    bool m_bIsReadFinish = {false};

    int64_t m_nTotalTime = {0};
    double m_dClock = {0};
    int m_nVolume = {SDL_MIX_MAXVOLUME};

    AVStream* m_pStream = {nullptr};
    AVCodecContext* m_pCodecCtx = {nullptr};
    AVPacket m_pkt;
    CAvPacketQueue m_quePkt;

    SDL_AudioSpec m_spec;
    uint8_t* m_audioBuf = {nullptr};
    uint32_t m_nAudioBufSize = {0};
    uint32_t m_nAudioBufIndex = {0};
    DECLARE_ALIGNED(16, uint8_t, m_audioBuf2)[192000];
    uint32_t m_nAudioBuf2Size = {0};

    uint32_t m_nAudioDevFmt = {AUDIO_F32SYS};
    uint32_t m_nAudioDepth = {0};
    SwrContext* m_pSwrCtx = {nullptr};
    int64_t m_nAudioDstChannelLayout = {0};
    AVSampleFormat m_enAudioDstFmt = {AV_SAMPLE_FMT_NONE};

    int64_t m_nAudioSrcChannelLayout = {0};
    int32_t m_nAudioSrcChannels = {0};
    AVSampleFormat m_enAudioSrcFmt = {AV_SAMPLE_FMT_NONE};
    int32_t m_nAudioSrcFreq = {0};

    int32_t m_nSendReturn = {0};

    float m_fSpeed = {1};
};

#endif // AUDIODECODER_H
