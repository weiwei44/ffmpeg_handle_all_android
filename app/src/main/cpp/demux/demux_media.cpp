//
// Created by BMW on 2018/8/30.
//

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include "demux_media.h"
#include "AndroidLog.h"

/**
 * 将分数转为浮点数
 * @param r
 * @return
 */
static double r2d(AVRational r) {
    //如果分子或者分母为0就返回0
    return r.num == 0 || r.den == 0 ? 0.0 : (double) r.num / (double) r.den;
}

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
    // 结合ffmpeg实例总结，此处不需要guess输出流封装格式，avformat_alloc_output_context2会调用
    // 输出流格式 flv rtmp流 ，mpegts裸流
   // int ret = avformat_alloc_output_context2(&outputContext,NULL,"mpegts",dstFilePath);
    AVOutputFormat *fmt = av_guess_format(NULL, dstFilePath, NULL);
    LOGE("fmt name = %s",fmt->name);
    int ret = avformat_alloc_output_context2(&outputContext,NULL,fmt->name,dstFilePath);

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


static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    LOGE("%s ----  pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d", tag,
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base), pkt->stream_index);
}
void DemuxMedia::ffmpegExp() {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;  //保存每个流的index
    int stream_mapping_size = 0; //流数量
    int packetCount = 0;

    if((ret = avformat_open_input(&ifmt_ctx,filePath,0,0)) < 0){
        LOGE("avformat_open_input error");
        goto end;
    }

    if((ret = avformat_find_stream_info(ifmt_ctx,0)) < 0){
        LOGE("avformat_find_stream_info error");
        goto end;
    }

    //开启调试信息
    av_dump_format(ifmt_ctx,0,filePath,0);

    // 创建输出上下文
    avformat_alloc_output_context2(&ofmt_ctx,NULL,NULL,dstFilePath);
    if (!ofmt_ctx) {
        LOGE("avformat_alloc_output_context2 failed");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    // 分配nmemb*size个字节的内存，并将分配的内存设置为0
    stream_mapping = static_cast<int *>(av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping)));
    if (!stream_mapping) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    ofmt = ofmt_ctx->oformat;
    for(i = 0; i < ifmt_ctx->nb_streams;i++) {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            // 未知流
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        //新建输出流
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            LOGE( "Failed allocating output stream");
            ret = AVERROR_UNKNOWN;
            goto end;
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            LOGE("Failed to copy codec parameters");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }

    av_dump_format(ofmt_ctx, 0, dstFilePath, 1);
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        // Io操作,打开输出文件，创建一个AVIOContext赋值给ofmt_ctx->pb
        ret = avio_open2(&ofmt_ctx->pb,dstFilePath,AVIO_FLAG_WRITE,0,0);
        //ret = avio_open(&ofmt_ctx->pb, dstFilePath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("Could not open output file '%s'", dstFilePath);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        LOGE("Error occurred when opening output file\n");
        goto end;
    }

    while(true){
        AVStream *in_stream, *out_stream;
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            break;

        in_stream  = ifmt_ctx->streams[pkt.stream_index];
        // 未知流信息直接释放
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0) {
            av_packet_unref(&pkt);
            continue;
        }


        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        // log_packet(ifmt_ctx, &pkt, "in");

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;

        /********************************音视频剪切***********************************/
        int cut_start = 10;  //从第10秒开始减
        int cut_time  = 30;  //剪切30秒

        int64_t cut_start_pts = (int64_t)(cut_start / r2d(out_stream->time_base));
        int64_t cut_time_pts = (int64_t)(cut_time / r2d(out_stream->time_base));  // 剪去的时间戳

        int pos = (int) (pkt.pts * r2d(out_stream->time_base));

        if(pkt.pts > cut_start_pts && pkt.pts  <= (cut_start_pts + cut_time_pts) )
        {
            av_packet_unref(&pkt);
            continue;
        }

        if(pkt.pts > cut_start_pts ){
            // dts不能小于剪切段的，因为视频没法保证当前剪切到的是I帧，音频无所谓
            if(pkt.dts - cut_time_pts < cut_start_pts){
                av_packet_unref(&pkt);
                continue;
            }
            pkt.pts = pkt.pts - cut_time_pts;
            pkt.dts = pkt.dts - cut_time_pts;
        }
        LOGE("pos = %d,pkt = %lld,dts = %lld,cut_time_pts = %lld",pos,pkt.pts,pkt.dts,cut_time_pts);
        /********************************音视频剪切***********************************/

        //log_packet(ofmt_ctx, &pkt, "out");
        //在有多个流的情况下要用av_interleaved_write_frame
        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0) {
            LOGE( "Error muxing packet == %s",av_err2str(ret));
            break;
        }
        av_packet_unref(&pkt);
    }
    av_write_trailer(ofmt_ctx);
end:
    avformat_close_input(&ifmt_ctx);
    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF) {
        LOGE("Error occurred: %s\n", av_err2str(ret));
        return;
    }
}
