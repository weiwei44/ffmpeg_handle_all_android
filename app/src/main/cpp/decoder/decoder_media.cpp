//
// Created by BMW on 2018/9/3.
//



#include "AndroidLog.h"
#include "decoder_media.h"


static void Rotate90(const AVFrame* src, AVFrame* dst)
{
    int half_width = src->width >> 1;
    int half_height = src->height >> 1;

    int size = src->linesize[0] * src->height;
    int half_size = size >> 2;

    for (int j = 0, n = 0; j < src->width; j++) {
        int pos = size;
        for (int i = src->height - 1; i >= 0; i--) {
            pos -= src->linesize[0];
            dst->data[0][n++] = src->data[0][pos + j];
        }
    }

    for (int j = 0, n = 0; j < half_width; j++) {
        int pos = half_size;
        for (int i = half_height - 1; i >= 0; i--) {
            pos -= src->linesize[1];
            dst->data[1][n] = src->data[1][pos + j];
            dst->data[2][n++] = src->data[2][pos + j];
        }
    }


    dst->height = src->width;
    dst->width  = src->height;
}




static void Rotate180(const AVFrame* src, AVFrame* dst)
{
    int half_width = src->width >> 1;
    int half_height = src->height >> 1;


    int pos = src->linesize[0] * src->height;
    for (int i = 0, n = 0; i < src->height; i++) {
        pos -= src->linesize[0];
        for (int j = src->width - 1; j >= 0; j--) {
            dst->data[0][n++] = src->data[0][pos + j];
        }
    }

    pos = src->linesize[0] * src->height >> 2;
    for (int i = 0, n = 0; i < half_height; i++) {
        pos -= src->linesize[1];
        for (int j = half_width - 1; j >= 0; j--) {
            dst->data[1][n] = src->data[1][pos + j];
            dst->data[2][n++] = src->data[2][pos + j];
        }
    }


    dst->width  = src->width;
    dst->height = src->height;
}


static void Rotate270(const AVFrame* src, AVFrame* dst) {
    int half_width = src->linesize[0] >> 1;
    int half_height = src->height >> 1;


    for (int i = src->width - 1, n = 0; i >= 0; i--) {
        for (int j = 0, pos = 0; j < src->height; j++) {
            dst->data[0][n++] = src->data[0][pos + i];
            pos += src->linesize[0];
        }
    }


    for (int i = (src->width >> 1) - 1, n = 0; i >= 0; i--) {
        for (int j = 0, pos = 0; j < half_height; j++) {
            dst->data[1][n] = src->data[1][pos + i];
            dst->data[2][n++] = src->data[2][pos + i];
            pos += half_width;
        }
    }


    dst->width = src->height;
    dst->height = src->width;
}

static int getRotateAngle(AVStream* avStream)

{
    AVDictionaryEntry *tag = NULL;
    int   m_Rotate=-1;
    tag = av_dict_get(avStream->metadata, "rotate", tag, 0);
    if (tag==NULL)
    {
        m_Rotate = 0;
    }
    else
    {
        int angle = atoi(tag->value);
        angle %= 360;
        if (angle == 90)
        {
            m_Rotate = 90;
        }
        else if (angle == 180)
        {
            m_Rotate = 180;
        }
        else if (angle == 270)
        {
            m_Rotate = 270;
        }
        else
        {
            m_Rotate = 0;

        }

    }

    return m_Rotate;

}

static void decode_video(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,AVFrame *pFrame,SwsContext *vctx,
                         FILE *outfile)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        LOGE("Error submitting the packet to the decoder\n");
        av_packet_unref(pkt);
        av_frame_unref(frame);
        av_frame_unref(pFrame);
        return;
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(pkt);
            av_frame_unref(frame);
            av_frame_unref(pFrame);
            return;
        }else if (ret < 0) {
            av_packet_unref(pkt);
            av_frame_unref(frame);
            av_frame_unref(pFrame);
            LOGE("Error during decoding\n");
            return;
        }


        int h = sws_scale(vctx,
                          (const uint8_t *const *) frame->data, frame->linesize, 0, dec_ctx->height,
                          pFrame->data, pFrame->linesize);
        LOGE("width = %d,height = %d",pFrame->width,pFrame->height);
        //write YUV
        int outWidth = 720;
        int outHeight = 1280;
        int y_size = outWidth * outHeight;
        fwrite(pFrame->data[0], 1, y_size, outfile);
        fwrite(pFrame->data[1], 1, y_size / 4, outfile);
        fwrite(pFrame->data[2],1, y_size / 4, outfile);
    }
}

static void decode_audio(AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame,
                   FILE *outfile)
{
    int i, channels;
    int ret, data_size;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) {
        LOGE("Error submitting the packet to the decoder\n");
        av_packet_unref(pkt);
        av_frame_unref(frame);
        return;
    }

    /* read all the output frames (in general there may be any number of them */
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_unref(pkt);
            av_frame_unref(frame);
            return;
        }else if (ret < 0) {
            av_packet_unref(pkt);
            av_frame_unref(frame);
            LOGE("Error during decoding\n");
            return;
        }

        // 重采样一下AV_SAMPLE_FMT_S16，不然你要打印出pcm位宽，通道，采样率格式播放，随意设置格式播放出来是杂音
        SwrContext* swr_ctx = NULL;
        swr_ctx = swr_alloc_set_opts(NULL,
                                     AV_CH_LAYOUT_STEREO,AV_SAMPLE_FMT_S16,frame->sample_rate,
                                     frame->channel_layout, (AVSampleFormat) frame->format, frame->sample_rate,
                                     NULL, NULL);
        if(!swr_ctx || swr_init(swr_ctx) < 0){
            av_packet_unref(pkt);
            av_frame_unref(frame);
            swr_free(&swr_ctx);
            swr_ctx = NULL;
            return;
        }

        // 分配一秒的采样大小, 采样率 * 2个字节位宽 * 2通道
        uint8_t *buff = (uint8_t*)av_malloc(frame->sample_rate * 2 * 2); // buff最好参数传进来，免得多次分配
        int nb = swr_convert(swr_ctx,&buff,frame->nb_samples,
                             (const uint8_t**)frame->data,frame->nb_samples);

        data_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
        if (data_size < 0) {
            /* This should not occur, checking just for paranoia */
            av_packet_unref(pkt);
            av_frame_unref(frame);
            av_free(buff);
            LOGE( "Failed to calculate data size\n");
            return;
        }

        // 重采样的数据大小 * 2通道 * AV_SAMPLE_FMT_S16位宽
        int result = nb * 2 * data_size;
        fwrite(buff,result,1,outfile);
        av_free(buff);
        //播放 ffplay -ar 44100 -channels 2 -f s16le -i ffmpegplay.pcm

//        for (i = 0; i < nb; i++)
//            for (channels = 0; channels < 2; channels++)
//                // packet格式, lrlrlrlr,交叉存
//                fwrite(frame->data[channels] + data_size*i, 1, data_size, outfile);
    }
}

void Decoder::ffmpegExp() {
    const AVCodec *audio_codec,*video_codec;
    AVCodecContext *audio_ctx,*video_ctx= NULL;
    int len, ret;
    AVFormatContext* pctx = NULL;

    pctx = avformat_alloc_context();
    avformat_open_input(&pctx,filePath,NULL,NULL);
    avformat_find_stream_info(pctx,NULL);

    audio_index = av_find_best_stream(pctx,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    video_index = av_find_best_stream(pctx,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
//    ret = av_dict_set(&pctx->streams[video_index]->metadata,"rotate","180",0); //设置旋转角度
//    LOGE("ret = %d",ret);
//    AVDictionaryEntry *m = NULL;
//    while((m=av_dict_get(pctx->streams[video_index]->metadata,"",m,AV_DICT_IGNORE_SUFFIX))!=NULL){
//        LOGE("Key:%s ===value:%s\n", m->key,m->value);
//    }
    LOGE("rotaio = %d",getRotateAngle(pctx->streams[video_index]));

    audio_codec = avcodec_find_decoder(pctx->streams[audio_index]->codecpar->codec_id);
    audio_ctx = avcodec_alloc_context3(audio_codec);
    avcodec_parameters_to_context(audio_ctx,pctx->streams[audio_index]->codecpar);
    avcodec_open2(audio_ctx,audio_codec,NULL);

    video_codec = avcodec_find_decoder(pctx->streams[video_index]->codecpar->codec_id);
    video_ctx = avcodec_alloc_context3(video_codec);
    avcodec_parameters_to_context(video_ctx,pctx->streams[video_index]->codecpar);
    avcodec_open2(video_ctx,video_codec,NULL);

    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    while (true){
        ret = av_read_frame(pctx,packet);
        if(ret == 0){
            if(packet->stream_index == audio_index){
                // 音频解码
                // decode_audio(audio_ctx,packet,frame,outFile);
            }else if(packet->stream_index == video_index){

                //ret = av_dict_set(&pctx->streams[video_index]->metadata,"rotate","90",0);
                ret = av_dict_set(&pctx->streams[video_index]->metadata,"rotate","180",0); //设置旋转角度

                // 视频解码
                out_buffer = (uint8_t *) av_malloc
                        ((size_t) av_image_get_buffer_size(AV_PIX_FMT_YUV420P, outWidth, outHeight, 1));
                pFrame = av_frame_alloc(); //初始化一个frame用来存储我们新的像素格式的数据
                //初始化缓冲区
                av_image_fill_arrays(pFrame->data, pFrame->linesize, out_buffer,
                                     AV_PIX_FMT_YUV420P, outWidth, outHeight, 1);

                vctx = sws_getContext(
                        video_ctx->width, video_ctx->height, video_ctx->pix_fmt,
                        outWidth, outHeight, AV_PIX_FMT_YUV420P,
                        SWS_FAST_BILINEAR,
                        0, 0, 0);
                decode_video(video_ctx,packet,frame,pFrame,vctx,outFile);
            }else{
                av_packet_unref(packet);
                av_frame_unref(frame);
                av_frame_unref(pFrame);
                continue;
            }
        }else{
            av_packet_unref(packet);
            av_frame_unref(frame);
            av_frame_unref(pFrame);
            break;
        }
    }

    /* flush the decoder */
    packet->data = NULL;
    packet->size = 0;
//    decode_audio(audio_ctx,packet,frame,outFile);
//    decode_video(video_ctx,packet,frame,pFrame,vctx,outFile);

    sws_freeContext(vctx);
    fclose(outFile);
    avcodec_free_context(&audio_ctx);
    avcodec_free_context(&video_ctx);
    avformat_close_input(&pctx);

    av_packet_free(&packet);
    av_frame_free(&frame);
    av_frame_free(&pFrame);
}

void Decoder::init(const char *url, const char *dstUrl) {
    av_register_all(); //注册封装器
    avcodec_register_all(); //注册解码器
    avformat_network_init();  //初始化网络

    filePath = new char[strlen(url)+1];
    strcpy(filePath,url);

    dstFilePath = new char[strlen(dstUrl)+1];
    strcpy(dstFilePath,dstUrl);

    outFile = fopen(dstFilePath,"wb");
}
