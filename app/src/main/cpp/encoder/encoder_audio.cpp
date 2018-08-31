//
// Created by BMW on 2018/8/31.
// 待处理
//

#include "encoder_audio.h"
#include "AndroidLog.h"

static bool check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt){
    const enum AVSampleFormat *p = codec->sample_fmts;
    while (*p != AV_SAMPLE_FMT_NONE){
        if (*p == sample_fmt)
            return true;
        p++;
    }
    return false;
}

static int select_sample_rate(const AVCodec* codec){
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
            best_samplerate = *p;
        p++;
    }
    return best_samplerate;
}

static int select_channel_layout(const AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}


Encoder::Encoder(const char* url,const char* dstUrl) {
    av_register_all(); //注册封装器
    avcodec_register_all(); //注册解码器
    avformat_network_init();  //初始化网络

    inputUrl = new char[strlen(url)+1];
    strcpy(inputUrl,url);

    outputUrl = new char[strlen(dstUrl)+1];
    strcpy(outputUrl,dstUrl);

}

Encoder::~Encoder() {

}


void Encoder::encode(AVCodecContext *ctx, AVFrame *frame, AVPacket *pkt, FILE *output) {

    int ret;
    ret = avcodec_send_frame(ctx,frame);

    if(ret < 0){
        LOGE("Error sending the frame to the encoder\n");
        return;
    }

    while (ret >= 0){
        ret = avcodec_receive_packet(ctx,pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            return;
        else if (ret < 0) {
            LOGE("Error encoding audio frame\n");
            break;
        }

        fwrite(pkt->data, 1, pkt->size, output);
        av_packet_unref(pkt);
    }
}

void Encoder::start() {

    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        LOGE("Codec not found\n");
        return;
    }

    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE("Could not allocate audio codec context\n");
        return;
    }

    /* put sample parameters */
    c->bit_rate = 64000;

    c->sample_fmt = AV_SAMPLE_FMT_FLTP;

    if (!check_sample_fmt(codec, c->sample_fmt)) {
        LOGE("Encoder does not support sample format %s",
                av_get_sample_fmt_name(c->sample_fmt));
        return;
    }

    c->sample_rate    = select_sample_rate(codec);
    c->channel_layout = select_channel_layout(codec);
    c->channels       = av_get_channel_layout_nb_channels(c->channel_layout);

    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return;
    }

    in_file = fopen(inputUrl,"rb");
    f = fopen(outputUrl, "wb");
    if (!f) {
        LOGE( "Could not open %s\n", outputUrl);
        return;
    }

    pkt = av_packet_alloc();
    if (!pkt) {
        LOGE("could not allocate the packet\n");
        return;
    }

    /* frame containing input raw audio */
    frame = av_frame_alloc();
    if (!frame) {
        LOGE("Could not allocate audio frame\n");
        return;
    }

    frame->nb_samples     = c->frame_size;
    frame->format         = c->sample_fmt;
    frame->channel_layout = c->channel_layout;

    int ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        LOGE("Could not allocate audio data buffers\n");
        return;
    }

    /* encode a single tone sound */
    t = 0;
    tincr = 2 * M_PI * 440.0 / c->sample_rate;
    int i = 0 , j = 0 , k = 0;
    for (i = 0; i < 200; i++) {
        /* make sure the frame is writable -- makes a copy if the encoder
         * kept a reference internally */
        ret = av_frame_make_writable(frame);
        if (ret < 0)
            return;
        samples = (uint16_t*)frame->data[0];

        for (j = 0; j < c->frame_size; j++) {
            samples[2*j] = (int)(sin(t) * 10000);

            for (k = 1; k < c->channels; k++)
                samples[2*j + k] = samples[2*j];
            t += tincr;
        }
        encode(c, frame, pkt, f);
    }

    /* flush the encoder */
    encode(c, NULL, pkt, f);

    fclose(f);

    av_frame_free(&frame);
    av_packet_free(&pkt);
    avcodec_free_context(&c);

}
