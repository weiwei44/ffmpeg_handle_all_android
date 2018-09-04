#pragma once

extern "C"{
#include <libswresample/swresample.h>
#include <libavformat/avformat.h>

}
class Pcm2AAC
{
public:
    Pcm2AAC();
    virtual ~Pcm2AAC();

    bool Init(int pcm_Sample_rate, AVSampleFormat pcm_Sample_fmt, int pcm_Channels);
    void AddData(char *pData, int size);
    bool GetData(char *&pOutData, int &iSize);
    void AddADTS(int packetLen);
    char m_pOutData[1024 * 10];
    char m_Pcm[10240];
    int m_PcmSize;
    char *m_PcmPointer[8];
    AVSampleFormat m_PcmFormat;
    int m_PcmChannel;
    int m_PcmSampleRate;

    AVFrame * frame;
    AVPacket *packet;
    AVCodec *codec;
    AVCodecContext *c = NULL;
    SwrContext *resample_context = NULL;
    int64_t pts;
};

