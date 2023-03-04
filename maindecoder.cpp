#include "maindecoder.h"
const double MIN_DOUBLE_VALUE = 1e-8;

CMainDecoder::CMainDecoder()
{
    av_init_packet(&m_seekPkt);
    m_seekPkt.data = (uint8_t*) "FLUSH";
    m_sptrAudioDecoder = make_shared<CAudioDecoder>();

    connect(m_sptrAudioDecoder.get(), SIGNAL(sigPlayFinished()), this, SLOT(sltFinishedAudio()));
    connect(this, SIGNAL(sigReadFinished()), m_sptrAudioDecoder.get(), SLOT(sltReadFileFinished()));
}

CMainDecoder::~CMainDecoder()
{
    if(m_pFltGph)
    {
        avfilter_graph_free(&m_pFltGph);
    }
}

double CMainDecoder::GetCurrentTime() const
{
    return m_nAudioIndex >= 0 ? m_sptrAudioDecoder->GetAudioClock() : 0;
}

void CMainDecoder::SeekProgress(const uint64_t &_nPos)
{
    if(false == m_bIsSeek)
    {
        m_nSeekPos = static_cast<int64_t>(_nPos);
        m_bIsSeek = true;
    }
}

int CMainDecoder::GetVolume() const
{
    return m_sptrAudioDecoder->GetVolume();
}

void CMainDecoder::SetVolume(const int &_nVolume)
{
    m_sptrAudioDecoder->SetVolume(_nVolume);
}

void CMainDecoder::SetSpeed(const float &_fValue)
{
    m_sptrAudioDecoder->SetSpeed(_fValue);
}

int CMainDecoder::initFilter()
{
    AVFilterInOut* pFltOut = avfilter_inout_alloc();
    AVFilterInOut* pFltIn = avfilter_inout_alloc();
    enum AVPixelFormat szPixFmts[] = {AV_PIX_FMT_RGB32, AV_PIX_FMT_NONE};
    if(m_pFltGph)
    {
        avfilter_graph_free(&m_pFltGph);
    }
    m_pFltGph = avfilter_graph_alloc();
    QString strFilter("pp=hb/vb/dr/al");
    QString strArgs = QString("video_size=%1x%2:pix_fmt=%3:time_base=%4/%5:pixel_aspect=%6/%7")
            .arg(m_pCodecCtx->width).arg(m_pCodecCtx->height).arg(m_pCodecCtx->pix_fmt)
            .arg(m_pVideoStream->time_base.num).arg(m_pVideoStream->time_base.den)
            .arg(m_pCodecCtx->sample_aspect_ratio.num).arg(m_pCodecCtx->sample_aspect_ratio.den);

    int nRet = avfilter_graph_create_filter(&m_pSrcFilCtx, avfilter_get_by_name("buffer"), "in", strArgs.toLocal8Bit().data(), nullptr, m_pFltGph);
    if(nRet < 0)
    {
        cout << "avfilter create failed, error:" << nRet;
        goto end;
    }

    nRet = avfilter_graph_create_filter(&m_pSinkFilCtx, avfilter_get_by_name("buffersink"), "out", nullptr, nullptr, m_pFltGph);
    if(nRet < 0)
    {
        cout << "avfilter create failed, error:" << nRet;
        goto end;
    }

    //set sink filter ouput format
    nRet = av_opt_set_int_list(m_pSinkFilCtx, "pix_fmts", szPixFmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
    if(nRet < 0)
    {
        cout << "av_opt_set_int_list failed,error:" << nRet;
        avfilter_graph_free(&m_pFltGph);
        goto end;
    }

    pFltOut->name = av_strdup("in");
    pFltOut->filter_ctx = m_pSrcFilCtx;
    pFltOut->pad_idx = 0;
    pFltOut->next = nullptr;

    pFltIn->name = av_strdup("out");
    pFltIn->filter_ctx = m_pSinkFilCtx;
    pFltIn->pad_idx = 0;
    pFltIn->next = nullptr;

    if(strFilter.isEmpty() || strFilter.isNull())
    {
        //if no filter to ad, just link source and sink
        nRet = avfilter_link(m_pSrcFilCtx, 0, m_pSinkFilCtx, 0);
        if(0 != nRet)
        {
            cout << "source filter link sink filter failed,error:" << nRet;
            goto end;
        }
    }
    else
    {
        //add filter to graph
        nRet = avfilter_graph_parse_ptr(m_pFltGph, strFilter.toLatin1().data(), &pFltIn, &pFltOut, nullptr);
        if(nRet < 0)
        {
            cout << "avfilter graph pase ptr failed, ret:" << nRet;
            goto end;
        }
    }

    //check validity and configure all the links and formats in the graph
    nRet = avfilter_graph_config(m_pFltGph, nullptr);
    if(nRet < 0)
    {
        cout << "avfilter_graph_config failed,error:" << nRet;
    }

    end:
    avfilter_inout_free(&pFltIn);
    avfilter_inout_free(&pFltOut);
    return nRet;
}

void CMainDecoder::run()
{
    int nSeekIndex = 0;
    m_pFmtCtx = avformat_alloc_context();
    int nRet = avformat_open_input(&m_pFmtCtx, m_strCurrFile.toLocal8Bit().data(), nullptr, nullptr);
    if(nRet != 0)
    {
        emit sigOpenFileError();
        cout << "open file:" << m_strCurrFile << " failed, error:" << nRet;
        return;
    }
    nRet = avformat_find_stream_info(m_pFmtCtx, nullptr);
    if(nRet < 0)
    {
        cout << "avformat_find_stream_info failed, error:" << nRet;
        avformat_free_context(m_pFmtCtx);
    }

    const bool& bIsRTSP = isRealTime(m_pFmtCtx);

    for(uint32_t i = 0; i < m_pFmtCtx->nb_streams; ++i)
    {
        if(AVMEDIA_TYPE_VIDEO == m_pFmtCtx->streams[i]->codecpar->codec_type)
        {
            m_nVideoIndex = static_cast<int32_t>(i);
        }
        else if(AVMEDIA_TYPE_AUDIO == m_pFmtCtx->streams[i]->codecpar->codec_type)
        {
            m_nAudioIndex = static_cast<int32_t>(i);
        }
        else if(AVMEDIA_TYPE_SUBTITLE == m_pFmtCtx->streams[i]->codecpar->codec_type)
        {
            m_nSubtitileIndex = static_cast<int32_t>(i);
        }
    }

    if("video" == m_strCurrType)
    {
        if(m_nVideoIndex < 0)
        {
            cout << "not support this file, video index:" << m_nVideoIndex << " audio index:" << m_nAudioIndex;
            avformat_free_context(m_pFmtCtx);
            return ;
        }
    }
    else
    {
        if(m_nAudioIndex < 0)
        {
            cout << "not support this file, video index:" << m_nVideoIndex << " audio index:" << m_nAudioIndex;
            avformat_free_context(m_pFmtCtx);
            return ;
        }
    }

    if(bIsRTSP)
    {
        emit sigGetVideoTime(0);
    }
    else
    {
        emit sigGetVideoTime(m_pFmtCtx->duration);
        m_nTotalTime = m_pFmtCtx->duration;
    }

    if(m_nAudioIndex >= 0)
    {
        if(m_sptrAudioDecoder->OpenAudio(m_pFmtCtx, m_nAudioIndex) < 0)
        {
            cout << "open audio codec and register callback funtion failed";
            avformat_free_context(m_pFmtCtx);
            return;
        }
    }

    if("video" == m_strCurrType)
    {
        m_pCodecCtx = avcodec_alloc_context3(nullptr);
        nRet = avcodec_parameters_to_context(m_pCodecCtx, m_pFmtCtx->streams[m_nVideoIndex]->codecpar);
        AVCodec* pCodec = avcodec_find_decoder(m_pCodecCtx->codec_id);
        if(nullptr == pCodec || nRet < 0)
        {
            cout << "find video decodec failed";
            avformat_free_context(m_pFmtCtx);
            goto end;
        }

        nRet = avcodec_open2(m_pCodecCtx, pCodec, nullptr);
        if(nRet < 0)
        {
            cout << "open video codec failed";
            goto end;
        }

        m_pVideoStream = m_pFmtCtx->streams[m_nVideoIndex];

        if(initFilter() < 0)
        {
            goto end;
        }

        SDL_CreateThread(&CMainDecoder::videoThread, "video_thread", this);
    }

    setPlayStat(EN_PLAYING);
    AVPacket pkt;

    while(true)
    {
        if(m_bIsStop)
        {
            break;
        }
        if(m_bIsPause)
        {
            SDL_Delay(10);
            continue;
        }

    seek:
        if(m_bIsSeek)
        {
            if("video" == m_strCurrType)
            {
               nSeekIndex = m_nVideoIndex;
            }
            else
            {
                nSeekIndex = m_nAudioIndex;
            }
            AVRational avRational = av_get_time_base_q();
            m_nSeekPos = av_rescale_q(m_nSeekPos, avRational, m_pFmtCtx->streams[nSeekIndex]->time_base);

            if(av_seek_frame(m_pFmtCtx, nSeekIndex, m_nSeekPos, AVSEEK_FLAG_ANY) < 0)
            {
                cout << "seek failed";
            }
            else
            {
                m_sptrAudioDecoder->ClearAudioData();
                m_sptrAudioDecoder->PacketEnqueue(m_seekPkt);

                if("video" == m_strCurrType)
                {
                    m_queVideo.Clear();
                    m_queVideo.Push(m_seekPkt);
                    m_dVideoClk = 0;
                }
            }
            m_bIsSeek = false;
        }

        if("video" == m_strCurrType && m_queVideo.Size() > 512)
        {
            SDL_Delay(10);
            continue;
        }

        //judge haven't read all frame

        if(av_read_frame(m_pFmtCtx, &pkt) < 0)
        {
            cout << "read file completed";
            m_bIsReadFinished = true;
            emit sigReadFinished();
            SDL_Delay(10);
            break;
        }

        if("video" == m_strCurrType && pkt.stream_index == m_nVideoIndex)
        {
            m_queVideo.Push(pkt);
        }
        else if(pkt.stream_index == m_nAudioIndex)
        {
            m_sptrAudioDecoder->PacketEnqueue(pkt);
        }
        else if(pkt.stream_index == m_nSubtitileIndex)
        {
            m_queSubtitle.Push(pkt);
            av_packet_unref(&pkt);
        }
        else
        {
            av_packet_unref(&pkt);
        }
    }

    while(false == m_bIsStop)
    {
        if(m_bIsSeek)
            goto seek;
        SDL_Delay(10);
    }

    end:
    if(m_nAudioIndex >=0)
    {
        m_sptrAudioDecoder->CloseAudio();
    }

    if("video" == m_strCurrType)
    {
        avcodec_close(m_pCodecCtx);
        avcodec_free_context(&m_pCodecCtx);
    }

    avformat_close_input(&m_pFmtCtx);
    avformat_free_context(m_pFmtCtx);

    m_bIsReadFinished = true;

    if("music" == m_strCurrType)
    {
        setPlayStat(EN_STOP);
    }

}

int CMainDecoder::videoThread(void *_pThis)
{
    int nRet = 0;
    double dPts = 0;
    AVPacket pkt;
    //av_init_packet(&pkt);
    AVFrame* pFrame = av_frame_alloc();
    CMainDecoder* pThis = static_cast<CMainDecoder*>(_pThis);

    while(true)
    {
        if(pThis->m_bIsStop)
        {
            break;
        }
        if(pThis->m_bIsPause)
        {
            SDL_Delay(10);
            continue;
        }

        if(pThis->m_queVideo.IsEmpty() && pThis->m_bIsReadFinished)
        {
             break;
        }

        pThis->m_queVideo.Pop(pkt);

        //flush codec buffer while recevied flush packet
        if(!strcmp((char*)pkt.data, "FLUSH"))
        {
            avcodec_flush_buffers(pThis->m_pCodecCtx);
            av_packet_unref(&pkt);
            continue;
        }

        nRet = avcodec_send_packet(pThis->m_pCodecCtx, &pkt);
        if((nRet < 0) && (nRet != AVERROR_EOF) && (nRet != AVERROR(EAGAIN)))
        {
            cout << "video send to decoder failed, error:" << nRet;
            av_packet_unref(&pkt);
            continue;
        }

        nRet = avcodec_receive_frame(pThis->m_pCodecCtx, pFrame);
        if(nRet < 0 && nRet != AVERROR_EOF)
        {
            cout << "video frame decode failed, error:" << nRet;
            av_packet_unref(&pkt);
            continue;
        }

        if((dPts = pFrame->pts) == AV_NOPTS_VALUE)
        {
            dPts = 0;
        }

        //video sync audio
        dPts *= av_q2d(pThis->m_pVideoStream->time_base);
        dPts = pThis->synchonize(pFrame, dPts);

        if(pThis->m_nAudioIndex >= 0)
        {
            while(true)
            {
                if(pThis->m_bIsStop)
                {
                    break;
                }

                double dAudioClk = pThis->m_sptrAudioDecoder->GetAudioClock();
                dPts = pThis->m_dVideoClk;

                if(dPts <= dAudioClk)
                {
                    break;
                }
                int nDelayTime = (dPts - dAudioClk) * 1000;
                nDelayTime = nDelayTime > 5 ? 5 : nDelayTime;
                SDL_Delay(nDelayTime);
            }
        }

        if(av_buffersrc_add_frame(pThis->m_pSrcFilCtx, pFrame) < 0)
        {
            cout << "av_buffersrc_add_frame failed";
            av_packet_unref(&pkt);
            continue;
        }
        if(av_buffersink_get_frame(pThis->m_pSinkFilCtx, pFrame) < 0)
        {
            cout << "av_buffersink_get_frame failed";
            av_packet_unref(&pkt);
            continue;
        }

        //rendering

        QImage tmpImg(pFrame->data[0], pThis->m_pCodecCtx->width, pThis->m_pCodecCtx->height, QImage::Format_RGB32);
        QImage img = tmpImg.copy();
        pThis->displayVideo(img);

        av_frame_unref(pFrame);
        av_packet_unref(&pkt);
    }

    av_frame_free(&pFrame);
    if(!pThis->m_bIsStop)
    {
        pThis->m_bIsStop = true;
    }

    SDL_Delay(100);
    pThis->m_bIsDecodeFinished = true;

    if(pThis->m_bGotStop)
    {
        pThis->setPlayStat(EN_STOP);
    }
    else
    {
        pThis->setPlayStat(EN_FINISH);
    }

    return 0;
}
void CMainDecoder::clearData()
{
    m_nVideoIndex = -1;
    m_nAudioIndex = -1;
    m_nSubtitileIndex = -1;
    m_nTotalTime = 0;
    m_dVideoClk = 0;

    m_bIsStop = false;
    m_bIsPause = false;
    m_bIsSeek = false;
    m_bIsDecodeFinished = false;
    m_bIsReadFinished = false;

    m_queVideo.Clear();
    m_sptrAudioDecoder->ClearAudioData();
}

void CMainDecoder::setPlayStat(const CMainDecoder::EnPlayState &_enState)
{
    emit sigPlayStateChanged(_enState);
    m_enPlayState = _enState;
}

void CMainDecoder::displayVideo(QImage _img)
{
    emit sigGetVideo(_img);
}

double CMainDecoder::synchonize(AVFrame *_pFrame, double _dPts)
{
    if(abs(_dPts) > MIN_DOUBLE_VALUE)
    {
        m_dVideoClk = _dPts;
    }
    else
    {
        _dPts = m_dVideoClk;
    }
    double dDelay = av_q2d(m_pCodecCtx->time_base);
    dDelay += _pFrame->repeat_pict * (dDelay * 0.5);
    m_dVideoClk += dDelay;
    return _dPts;
}

bool CMainDecoder::isRealTime(AVFormatContext *_pFmtCtx)
{
    if(!strcmp(_pFmtCtx->iformat->name, "rtp")  ||
       !strcmp(_pFmtCtx->iformat->name, "rtsp") ||
       !strcmp(_pFmtCtx->iformat->name, "sdp"))
    {
        return true;
    }
    return false;
}

void CMainDecoder::sltDecoderFile(QString _strFile, QString _strType)
{
    cout << "fille name:" << _strFile << " ,type:" << _strType << " ,playState:" << m_enPlayState;
    if(m_enPlayState != EN_STOP)
    {
        m_bIsStop = true;
        while(m_enPlayState != EN_STOP)
        {
            SDL_Delay(10);
        }
        SDL_Delay(100);
    }
    clearData();
    SDL_Delay(100);
    m_strCurrFile = _strFile;
    m_strCurrType = _strType;

    this->start();
}

void CMainDecoder::sltStopVideo()
{
    if(EN_STOP == m_enPlayState)
    {
        setPlayStat(EN_STOP);
        return;
    }

    m_bGotStop = true;
    m_bIsStop = true;
    m_sptrAudioDecoder->StopAudio();

    if("video" == m_strCurrType)
    {
        while(!m_bIsReadFinished || !m_bIsDecodeFinished)
        {
            SDL_Delay(10);
        }
    }
    else
    {
        while(!m_bIsReadFinished)
        {
            SDL_Delay(10);
        }
    }
}

void CMainDecoder::sltPauseVideo()
{
    if(EN_STOP == m_enPlayState)
    {
        return;
    }

    m_bIsPause = !m_bIsPause;
    m_sptrAudioDecoder->PauseAudio();
    if(m_bIsPause)
    {
        av_read_pause(m_pFmtCtx);
        setPlayStat(EN_PAUSE);
    }
    else
    {
        av_read_play(m_pFmtCtx);
        setPlayStat(EN_PLAYING);
    }
}

void CMainDecoder::sltFinishedAudio()
{
    m_bIsStop = true;
    if("music" == m_strCurrType)
    {
        SDL_Delay(10);
        emit sigPlayStateChanged(EN_FINISH);
    }
}
