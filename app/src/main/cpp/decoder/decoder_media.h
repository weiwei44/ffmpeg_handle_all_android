//
// Created by BMW on 2018/9/3.
//

#ifndef FFMPEGMASTER_DECODER_MEDIA_H
#define FFMPEGMASTER_DECODER_MEDIA_H

#include <string>
extern "C"{
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
};
class Decoder {

public:
    Decoder(){}
    ~Decoder(){}
    void init(const char* url,const char* dstUrl);

    void ffmpegExp();

private:
    int audio_index = -1;
    int video_index = -1;

    char* filePath = NULL;
    char* dstFilePath = NULL;

    FILE* outFile = NULL;

    // 格式转换后的宽高
    int outWidth = 720;  //这里高必须大于宽，不然格式转换不成功
    int outHeight = 1280;
    uint8_t *out_buffer = NULL;
    SwsContext *vctx = NULL;
    AVFrame *pFrame = NULL;
};


#endif //FFMPEGMASTER_DECODER_MEDIA_H
