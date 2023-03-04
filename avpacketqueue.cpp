#include "avpacketqueue.h"

CAvPacketQueue::CAvPacketQueue()
{
    m_pMutex = SDL_CreateMutex();
    m_pCond = SDL_CreateCond();
}

CAvPacketQueue::~CAvPacketQueue()
{
    SDL_DestroyMutex(m_pMutex);
    SDL_DestroyCond(m_pCond);
}

void CAvPacketQueue::Push(const AVPacket& _pkt)
{
    SDL_LockMutex(m_pMutex);
    m_que.enqueue(_pkt);
    SDL_CondSignal(m_pCond);
    SDL_UnlockMutex(m_pMutex);
}

void CAvPacketQueue::Pop(AVPacket& _pkt, bool _bIsBolck)
{
    SDL_LockMutex(m_pMutex);
    while(true)
    {
        if(false == m_que.isEmpty())
        {
            _pkt = m_que.dequeue();
            break;
        }
        if(false == _bIsBolck)
        {
           break;
        }
        else
        {
            SDL_CondWait(m_pCond, m_pMutex);
        }
    }
    SDL_UnlockMutex(m_pMutex);
}

void CAvPacketQueue::Clear()
{
    SDL_LockMutex(m_pMutex);
    while(false == m_que.isEmpty())
    {
        AVPacket pkt = m_que.dequeue();
        av_packet_unref(&pkt);
    }
    SDL_UnlockMutex(m_pMutex);
}

bool CAvPacketQueue::IsEmpty() const
{
    return m_que.isEmpty();
}

int CAvPacketQueue::Size() const
{
    return m_que.size();
}
