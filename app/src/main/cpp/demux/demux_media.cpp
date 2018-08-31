//
// Created by BMW on 2018/8/30.
//


#include "demux_media.h"
#include "AndroidLog.h"

static int interrupt_cb(void *ctx)
{
    DemuxMedia* demuxMedia = static_cast<DemuxMedia *>(ctx);
    int  timeout  = 10;  //10秒超时
    if(av_gettime() - demuxMedia->lastReadPacktTime > timeout *1000 *1000)
    {
        LOGE("timeout");
        return -1;
    }
    return 0;
}

DemuxMedia::DemuxMedia() {

}

DemuxMedia::~DemuxMedia() {

}

int DemuxMedia::openInput() {
    inputContext = avformat_alloc_context();
    AVDictionary* options = NULL;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);//采用tcp传输
    av_dict_set(&options, "stimeout", "2000000", 0);//如果没有设置stimeout，无网状态，av_read_frame会阻塞（时间单位是微妙）
    lastReadPacktTime = av_gettime();
    inputContext->interrupt_callback.callback = interrupt_cb;
    inputContext->interrupt_callback.opaque = this;

    int ret = avformat_open_input(&inputContext,filePath,0,&options);
    if(ret != 0){
        char buf[1024] = {0};
        av_strerror(ret,buf, sizeof(buf));
        LOGE("FFDemux avformat_open_input %s , failed!，%s",buf,filePath);
        return ret;
    }

    //读取文件信息
    ret = avformat_find_stream_info(inputContext,0);
    if(ret != 0){
        char buf[1024] = {0};
        av_strerror(ret,buf, sizeof(buf));
        LOGE("FFDemux avformat_find_stream_info %s , failed!",buf);
        return ret;
    }

    return ret;
}

int DemuxMedia::openOutput() {
    // 输出流格式 flv rtmp流 ，mpegts裸流
   // int ret = avformat_alloc_output_context2(&outputContext,NULL,"mpegts",dstFilePath);
    int ret = avformat_alloc_output_context2(&outputContext,NULL,"flv",dstFilePath);

    if(ret < 0){
        char buf[1024] = {0};
        av_strerror(ret,buf, sizeof(buf));
        LOGE("FFDemux avformat_alloc_output_context2 %s , failed!",buf);
        return ret;
    }
    ret = avio_open2(&outputContext->pb,dstFilePath,AVIO_FLAG_WRITE,0,0);
    if(ret < 0){
        char buf[1024] = {0};
        av_strerror(ret,buf, sizeof(buf));
        LOGE("avio_open2 failed! %s",buf);
        return ret;
    }

    for(int i = 0;i<inputContext->nb_streams;i++){
        AVStream* stream = avformat_new_stream(outputContext,NULL);
        if(!stream){
            LOGE("avformat_new_stream failed");
        }
        ret = avcodec_parameters_copy(outputContext->streams[i]->codecpar, inputContext->streams[i]->codecpar);
        if(ret < 0)
        {
            LOGE("copy coddec context failed");
            goto Error;
        }
    }

    ret = avformat_write_header(outputContext, nullptr);
    if(ret < 0)
    {
        LOGE("format write header failed");
        goto Error;
    }

    LOGE(" Open output file success %s\n",dstFilePath);
    return ret ;
Error:
    if(outputContext)
    {
        avformat_close_input(&outputContext);
    }
    return ret ;
}

void DemuxMedia::closeInput() {
    if(inputContext != NULL){
        avformat_close_input(&inputContext);
    }
}

void DemuxMedia::closeOutput() {
    if(outputContext != NULL)
    {
        avformat_close_input(&outputContext);
    }
}

AVPacket* DemuxMedia::readPacketFromSource() {
    AVPacket *pkt = av_packet_alloc();
    lastReadPacktTime = av_gettime();
    int ret = av_read_frame(inputContext,pkt);
    if(ret >= 0){
        return pkt;
    }else{
        return NULL;
    }
}

int DemuxMedia::writePacket(AVPacket* packet) {
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    av_packet_rescale_ts(packet,inputStream->time_base,outputStream->time_base);
    return av_interleaved_write_frame(outputContext, packet);
}

void DemuxMedia::init(const char *url, const char *dstUrl) {
    av_register_all(); //注册封装器
    avcodec_register_all(); //注册解码器
    avformat_network_init();  //初始化网络

    filePath = new char[strlen(url)+1];
    strcpy(filePath,url);

    dstFilePath = new char[strlen(dstUrl)+1];
    strcpy(dstFilePath,dstUrl);
}
