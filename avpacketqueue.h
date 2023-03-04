#ifndef AVPACKETQUEUE_H
#define AVPACKETQUEUE_H
#include<QQueue>
#include "SDL2/SDL.h"
extern "C"
{
#include"libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

class CAvPacketQueue
{
public:
    explicit CAvPacketQueue();

    ~CAvPacketQueue();

    void Push(const AVPacket& _pkt);

    void Pop(AVPacket& _pkt, bool _bIsBolck=true);

    void Clear();

    bool IsEmpty() const;

    int Size() const;

private:
    QQueue<AVPacket> m_que;

    SDL_mutex* m_pMutex;

    SDL_cond* m_pCond;
};

#endif // AVPACKETQUEUE_H
