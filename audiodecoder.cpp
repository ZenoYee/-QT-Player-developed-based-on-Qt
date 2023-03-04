#include "audiodecoder.h"
#include <QtGlobal>
/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30

CAudioDecoder::CAudioDecoder(QObject *_pObj):QObject(_pObj)
{
   // SDL_Init(SDL_INIT_AUDIO);
}

CAudioDecoder::~CAudioDecoder()
{
    if(m_pCodecCtx)
    {
        avcodec_close(m_pCodecCtx);
        avcodec_free_context(&m_pCodecCtx);
    }
}

int CAudioDecoder::OpenAudio(AVFormatContext *_pFmtCtx,  const int32_t& _index)
{
    int nbChannels = 0;
    int nextNbChannels[] = {0, 0, 1, 6, 2, 6, 4, 6};
    int nextSampleRates[] = {0, 44100, 48000, 96000, 192000};
    int nextSampleRateIdx = FF_ARRAY_ELEMS(nextSampleRates) - 1;

    m_bIsStop = false;
    m_bIsPause = false;
    m_bIsReadFinish = false;
    m_enAudioSrcFmt = AV_SAMPLE_FMT_NONE;
    m_nAudioSrcFreq = 0;
    m_nAudioSrcChannelLayout = 0;

    _pFmtCtx->streams[_index]->discard = AVDISCARD_DEFAULT;
    m_pStream = _pFmtCtx->streams[_index];

    m_pCodecCtx = avcodec_alloc_context3(nullptr);
    avcodec_parameters_to_context(m_pCodecCtx, _pFmtCtx->streams[_index]->codecpar);
    AVCodec* pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
    if(nullptr == pCodec)
    {
        avcodec_free_context(&m_pCodecCtx);
        cout <<  " avcodec_find_decoder failed";
        return -1;
    }

    int nRet = avcodec_open2(m_pCodecCtx, pCodec, nullptr);
    if(nRet < 0)
    {
        avcodec_free_context(&m_pCodecCtx);
        cout << "audio codec open failed";
        return -1;
    }

    m_nTotalTime = _pFmtCtx->duration;

    const char* pEnv = SDL_getenv("SDL_AUDIO_CHANNELS");
    if(pEnv)
    {
        nbChannels = atoi(pEnv);
        m_nAudioDstChannelLayout = av_get_default_channel_layout(nbChannels);
    }

    nbChannels = m_pCodecCtx->channels;
    if(0 == m_nAudioDstChannelLayout ||
       nbChannels != av_get_channel_layout_nb_channels(m_nAudioDstChannelLayout))
    {
        m_nAudioDstChannelLayout = av_get_default_channel_layout(nbChannels);
        m_nAudioDstChannelLayout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
    }

    SDL_AudioSpec audioSpec;
    audioSpec.channels = static_cast<uint8_t>(av_get_channel_layout_nb_channels(m_nAudioDstChannelLayout));
    audioSpec.freq = m_pCodecCtx->sample_rate;

    if(audioSpec.channels <= 0 || audioSpec.freq <= 0)
    {
        cout << "invalid sample:" << audioSpec.freq << " or channels:" << audioSpec.channels;
        avcodec_free_context(&m_pCodecCtx);
        return -1;
    }

    while(nextSampleRateIdx && nextSampleRates[nextSampleRateIdx] >= audioSpec.freq)
    {
        nextSampleRateIdx--;
    }

    audioSpec.format = static_cast<uint16_t>(m_nAudioDevFmt);
    audioSpec.silence = 0;
    audioSpec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE, 2 << av_log2(audioSpec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
    audioSpec.callback = &CAudioDecoder::audioCallBack;
    audioSpec.userdata = this;

    while(true)
    {
        while(SDL_OpenAudio(&audioSpec, &m_spec) < 0)
        {
            cout << "SDL_OpenAudio failed,error:" << SDL_GetError();
            audioSpec.channels = nextNbChannels[FFMIN(7, audioSpec.channels)];
            if(0 == audioSpec.channels)
            {
                audioSpec.freq = nextSampleRates[nextSampleRateIdx--];
                audioSpec.channels = static_cast<uint8_t>(nbChannels);
                if(0 == audioSpec.freq)
                {
                    avcodec_free_context(&m_pCodecCtx);
                    cout << "no more combinations to try, audio open failed";
                    return -1;
                }
            }
            m_nAudioDstChannelLayout = av_get_default_channel_layout(audioSpec.channels);
        }

        if(m_spec.format != m_nAudioDevFmt)
        {
            cout << "SDL audio format:" << audioSpec.format << " is not supported"
                 << ", set to advised audio format:" << m_spec.format;
            audioSpec.format = m_spec.format;
            m_nAudioDevFmt = m_spec.format;
            SDL_CloseAudio();
        }
        else
        {
            break;
        }
    }

    if(m_spec.channels != audioSpec.channels)
    {
        m_nAudioDstChannelLayout = av_get_default_channel_layout(m_spec.channels);
        if(0 == m_nAudioDstChannelLayout)
        {
            cout << "SDL advised channels:" << m_spec.channels << " is not supported!";
            avcodec_free_context(&m_pCodecCtx);
            return -1;
        }
    }

    setSampleFormat(m_nAudioDevFmt);
    SDL_PauseAudio(0);
    return 0;
}

void CAudioDecoder::CloseAudio()
{
    ClearAudioData();

    SDL_LockAudio();
    SDL_CloseAudio();
    SDL_UnlockAudio();

    if(m_pCodecCtx)
    {
        avcodec_close(m_pCodecCtx);
        avcodec_free_context(&m_pCodecCtx);
    }
}

void CAudioDecoder::PauseAudio()
{
    m_bIsPause = !m_bIsPause;
}

void CAudioDecoder::StopAudio()
{
    m_bIsStop = true;
}

int CAudioDecoder::GetVolume() const
{
    return m_nVolume;
}

void CAudioDecoder::SetVolume(const int& _nVolume)
{
    m_nVolume = _nVolume;
}

void CAudioDecoder::SetSpeed(const float &_fValue)
{
    m_fSpeed = _fValue;
}

double CAudioDecoder::GetAudioClock()
{
    if(m_pCodecCtx)
    {
        int32_t nHwBufSize = m_nAudioBufSize - m_nAudioBufIndex;
        int32_t nBytesPerSec = m_pCodecCtx->sample_rate * m_pCodecCtx->channels * m_nAudioDepth;
        m_dClock -= static_cast<double>(nHwBufSize) / nBytesPerSec;
    }
    return m_dClock;
}

void CAudioDecoder::PacketEnqueue(const AVPacket &_pkt)
{
    m_quePkt.Push(_pkt);
}

void CAudioDecoder::ClearAudioData()
{
    m_audioBuf = nullptr;
    m_nAudioBufSize = 0;
    m_nAudioBuf2Size = 0;
    m_dClock = 0;
    m_nSendReturn = 0;
    m_quePkt.Clear();
}


int CAudioDecoder::decodeAudio()
{
    AVFrame* pFrame = av_frame_alloc();
    int nResampledDataSize = 0;

    if(nullptr == pFrame)
    {
        cout << "av_frame_alloc failed";
        return -1;
    }

    if(m_bIsStop)
    {
        av_frame_free(&pFrame);
        return -1;
    }

    if(m_quePkt.IsEmpty())
    {
        if(m_bIsReadFinish)
        {
            m_bIsStop = true;
            SDL_Delay(100);
            emit sigPlayFinished();
        }
        av_frame_free(&pFrame);
        return -1;
    }

    //get new packet while last packet all has been resolved
    if(m_nSendReturn != AVERROR(EAGAIN))
    {
        m_quePkt.Pop(m_pkt);
    }


    if(!strcmp((char*)m_pkt.data, "FLUSH"))
    {
        avcodec_flush_buffers(m_pCodecCtx);
        av_packet_unref(&m_pkt);
        av_frame_free(&pFrame);
        m_nSendReturn = 0;
        cout << "seek audio";
        return -1;
    }

    //while return -11 means packet have data not resolved, this packet can't be unref
    m_nSendReturn = avcodec_send_packet(m_pCodecCtx, &m_pkt);
    if(m_nSendReturn < 0 && m_nSendReturn != AVERROR(EAGAIN) && m_nSendReturn != AVERROR_EOF)
    {
        if(-11 != m_nSendReturn)
        {
            av_packet_unref(&m_pkt);
            av_frame_free(&pFrame);
        }

        cout << "audio frame decode failed？, error:" << m_nSendReturn;
        return m_nSendReturn;
    }

    int nRet = avcodec_receive_frame(m_pCodecCtx, pFrame);
    if(nRet < 0 && nRet != AVERROR(EAGAIN))
    {
        av_packet_unref(&m_pkt);
        av_frame_free(&pFrame);
        cout << "audio frame decode failed,error:" << nRet;
        return nRet;
    }

    if(pFrame->pts != AV_NOPTS_VALUE)
    {
        m_dClock = av_q2d(m_pStream->time_base) * pFrame->pts;
    }

    pFrame->sample_rate = qRound(pFrame->sample_rate * m_fSpeed);
    //get audio channels
    int64_t nInChannelLayout = (pFrame->channel_layout && pFrame->channels == av_get_channel_layout_nb_channels(pFrame->channel_layout)) ?
                pFrame->channel_layout : av_get_default_channel_layout(pFrame->channels);

    if( pFrame->format      != m_enAudioSrcFmt           ||
        pFrame->sample_rate != m_nAudioSrcFreq           ||
        nInChannelLayout    != m_nAudioSrcChannelLayout  ||
        nullptr == m_pSwrCtx)
    {
        if(m_pSwrCtx)
        {
            swr_free(&m_pSwrCtx);
        }

        //ini swr audio convert context
        m_pSwrCtx = swr_alloc_set_opts(nullptr, m_nAudioDstChannelLayout, m_enAudioDstFmt, m_spec.freq,
                                       nInChannelLayout, (AVSampleFormat)pFrame->format, pFrame->sample_rate, 0, nullptr);
        if(nullptr == m_pSwrCtx || swr_init(m_pSwrCtx) < 0)
        {
            av_packet_unref(&m_pkt);
            av_frame_free(&pFrame);
            return -1;
        }

        m_enAudioSrcFmt          = static_cast<AVSampleFormat>(pFrame->format);
        m_nAudioSrcFreq          = pFrame->sample_rate;
        m_nAudioSrcChannelLayout = nInChannelLayout;
        m_nAudioSrcChannels      = pFrame->channels;
    }

    if(m_pSwrCtx)
    {
        const uint8_t** in = const_cast<const uint8_t**>(pFrame->extended_data);
        uint8_t* out[] = {m_audioBuf2};
        int nOutCount = sizeof(m_audioBuf2) / m_spec.channels / av_get_bytes_per_sample(m_enAudioDstFmt);

        int nSampleSize = swr_convert(m_pSwrCtx, out, nOutCount, in, pFrame->nb_samples);

        if(nSampleSize < 0)
        {
            cout << "swr_convert failed:";
            av_packet_unref(&m_pkt);
            av_frame_free(&pFrame);
            return -1;
        }

        if(nSampleSize == nOutCount)
        {
            cout << "audio buffer is probably too small";
            if(swr_init(m_pSwrCtx) < 0)
            {
                swr_free(&m_pSwrCtx);
            }
        }

        m_audioBuf = m_audioBuf2;
        nResampledDataSize = nSampleSize * m_spec.channels * av_get_bytes_per_sample(m_enAudioDstFmt);
    }
    else
    {
        m_audioBuf = pFrame->data[0];
        nResampledDataSize = av_samples_get_buffer_size(nullptr, pFrame->channels, pFrame->nb_samples, static_cast<AVSampleFormat>(pFrame->format), 1);
    }

    m_dClock += static_cast<double>(nResampledDataSize) / (m_nAudioDepth * m_pCodecCtx->channels * m_pCodecCtx->sample_rate);

    if(m_nSendReturn != AVERROR(EAGAIN))
    {
        av_packet_unref(&m_pkt);
    }
    av_frame_free(&pFrame);
    return nResampledDataSize;
}

void CAudioDecoder::setSampleFormat(const uint32_t &_audioDecFmt)
{
    switch(_audioDecFmt)
    {
        case AUDIO_U8:
        {
            m_enAudioDstFmt = AV_SAMPLE_FMT_U8;
            m_nAudioDepth = 1;
        }
        break;

        case AUDIO_S16SYS:
        {
            m_enAudioDstFmt = AV_SAMPLE_FMT_S16;
            m_nAudioDepth = 2;
        }
        break;

        case AUDIO_S32SYS:
        {
            m_enAudioDstFmt = AV_SAMPLE_FMT_S32;
            m_nAudioDepth = 4;
        }
        break;

        case AUDIO_F32SYS:
        {
            m_enAudioDstFmt = AV_SAMPLE_FMT_FLT;
            m_nAudioDepth = 4;
        }
        break;

        default:
        {
            m_enAudioDstFmt = AV_SAMPLE_FMT_S16;
            m_nAudioDepth = 2;
        }
        break;
    }
}

void CAudioDecoder::audioCallBack(void *_pUserData, uint8_t *_pStream, int _SDL_AudioBuffSize)
{
    CAudioDecoder* pThis = static_cast<CAudioDecoder*>(_pUserData);
    while(_SDL_AudioBuffSize > 0)
    {
        if(pThis->m_bIsStop)
        {
            return;
        }

        if(pThis->m_bIsPause)
        {
            SDL_Delay(10);
            continue;
        }

        //no data in buffer
        if(pThis->m_nAudioBufIndex >= pThis->m_nAudioBufSize)
        {
            int nDecodeSize = pThis->decodeAudio();
            //if error, just output silence
            if(nDecodeSize < 0)
            {
                pThis->m_nAudioBufSize = 1024;
                pThis->m_audioBuf = nullptr;
            }
            else
            {
                pThis->m_nAudioBufSize = static_cast<uint32_t>(nDecodeSize);
            }
            pThis->m_nAudioBufIndex = 0;
        }

        //calculate how many data that haven't play
        uint32_t nSuplus = pThis->m_nAudioBufSize - pThis->m_nAudioBufIndex;
        if(nSuplus > _SDL_AudioBuffSize)
        {
            nSuplus = _SDL_AudioBuffSize;
        }

        if(pThis->m_audioBuf)
        {
            memset(_pStream, 0, nSuplus);
            SDL_MixAudio(_pStream, pThis->m_audioBuf + pThis->m_nAudioBufIndex, nSuplus, pThis->m_nVolume);
        }

        _SDL_AudioBuffSize -= nSuplus;
        _pStream += nSuplus;
        pThis->m_nAudioBufIndex += nSuplus;
    }
}


void CAudioDecoder::sltReadFileFinished()
{
    m_bIsReadFinish = true;
}
