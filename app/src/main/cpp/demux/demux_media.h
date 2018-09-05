//
// Created by BMW on 2018/8/30.
//

#ifndef FFMPEGMASTER_DEMUX_MEDIA_H
#define FFMPEGMASTER_DEMUX_MEDIA_H

extern "C"{
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
};

class DemuxMedia {

public:
    DemuxMedia();
    ~DemuxMedia();

    void init(const char* url,const char* dstUrl);
    int openInput();
    int openOutput(char* dstUrl = NULL);

    int initDecoder();
    int initEcoder();
    int initSws();

    bool decoder(AVPacket*,AVFrame*);
    bool ecoder(AVPacket*);

    AVPacket* readPacketFromSource();
    int writePacket(AVPacket*);

    void picFileName(char* file_name);

    void closeInput();
    void closeOutput();

    void save_pic1(char*path,AVPacket* pkt);

    void ffmpegExp();

public:
    int64_t lastReadPacktTime = 0;
    int audio_index = -1;
    int video_index = -1;

private:
    char* filePath = NULL;
    char* dstFilePath = NULL;

    AVFormatContext*  inputContext = 0;
    AVFormatContext* outputContext = 0;


    AVCodecContext* pDecoderCodecCtx = NULL;
    AVCodecContext* pEncoderCodecCtx = NULL;

    uint8_t * out_buffer= NULL;

    SwsContext*vctx = NULL;
    AVFrame* pFrame= NULL;

    int count = 0;

};


#endif //FFMPEGMASTER_DEMUX_MEDIA_H
