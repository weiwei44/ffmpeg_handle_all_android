//
// Created by BMW on 2018/8/31.
//

#ifndef FFMPEGMASTER_ENCODER_AUDIO_H
#define FFMPEGMASTER_ENCODER_AUDIO_H

#include <string>
extern "C"{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
};

class Encoder {
public:
    Encoder(const char* url,const char* dstUrl);
    ~Encoder();

   void encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt,
               FILE *output);

    void start();

private:
    FILE * f = NULL;
    FILE * in_file = NULL;

    int count = 0;
    int m_PcmSampleRate = 44100;

    char* inputUrl = NULL;
    char* outputUrl = NULL;

    AVCodec* codec = NULL;
    AVCodecContext* c = NULL;
    AVFormatContext* pFormatCtx = NULL;
    uint16_t *samples;
    float t, tincr;

    AVStream* audio_st= NULL;
    AVPacket* pkt = NULL;
    AVFrame* frame = NULL;

    SwrContext* swrContext = NULL;


    int64_t pts;

};


#endif //FFMPEGMASTER_ENCODER_AUDIO_H
