//
// Created by BMW on 2018/8/30.
//

#ifndef FFMPEGMASTER_DEMUX_MEDIA_H
#define FFMPEGMASTER_DEMUX_MEDIA_H

extern "C"{
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
};

class DemuxMedia {

public:
    DemuxMedia();
    ~DemuxMedia();

    void init(const char* url,const char* dstUrl);
    int openInput();
    int openOutput();

    AVPacket* readPacketFromSource();
    int writePacket(AVPacket*);

    void closeInput();
    void closeOutput();

    void ffmpegExp();

public:
    int64_t lastReadPacktTime = 0;

private:
    char* filePath = NULL;
    char* dstFilePath = NULL;

    AVFormatContext*  inputContext = 0;
    AVFormatContext* outputContext = 0;

    int audio_index = -1;
    int video_index = -1;



};


#endif //FFMPEGMASTER_DEMUX_MEDIA_H
