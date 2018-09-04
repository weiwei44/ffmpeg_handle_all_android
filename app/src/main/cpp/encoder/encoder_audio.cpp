#include "encoder_audio.h"
#include "AndroidLog.h"

Pcm2AAC::Pcm2AAC()
{
    for (int i = 0; i < 8; i++)
    {
        m_PcmPointer[i] = new char[10240];
    }
}


Pcm2AAC::~Pcm2AAC()
{
    avcodec_send_frame(c, NULL);
    char *pOut = NULL;
    int iSize = 0;
    while (GetData(pOut, iSize))
    {

    }
    for (int i = 0; i < 8; i++)
    {
        delete[] m_PcmPointer[i] ;
    }
    if (NULL != packet)
    {
        av_packet_free(&packet);
    }
    if (NULL != frame)
    {
        av_frame_free(&frame);
    }
    if (NULL != c)
    {
        avcodec_free_context(&c);
    }
    if (NULL != resample_context)
    {
        swr_free(&resample_context);
    }

}
bool Pcm2AAC::Init(int pcm_Sample_rate, AVSampleFormat pcm_Sample_fmt, int pcm_Channels)
{
    m_PcmFormat = pcm_Sample_fmt;
    m_PcmChannel = pcm_Channels;
    m_PcmSampleRate = pcm_Sample_rate;
    av_register_all();
    avcodec_register_all();
    codec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!codec) {
        LOGE("Codec not found\n");
        return false;
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        LOGE("Could not allocate audio codec context\n");
        return false;
    }
    c->channels = m_PcmChannel;
    c->channel_layout = av_get_default_channel_layout(m_PcmChannel);
    c->sample_rate = m_PcmSampleRate;
    c->sample_fmt = AV_SAMPLE_FMT_FLTP;//AV_SAMPLE_FMT_FLTP;
    c->bit_rate = 64000;
    /* Allow the use of the experimental AAC encoder. */
    c->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;


    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        LOGE( "Could not open codec\n");
        return false;
    }
    packet = av_packet_alloc();
    if (NULL == packet)
    {
        return false;
    }
    frame = av_frame_alloc();
    if (NULL == frame)
    {
        return false;
    }
    resample_context = swr_alloc_set_opts(NULL, c->channel_layout, c->sample_fmt,
                                          c->sample_rate, c->channel_layout, m_PcmFormat, c->sample_rate, 0, NULL);
    if (NULL == resample_context)
    {
        LOGE("Could not allocate resample context\n");
        return false;
    }
    if (swr_init(resample_context) < 0)
    {
        LOGE( "Could not open resample context\n");
        return false;
    }

    return true;
}
void Pcm2AAC::AddData(char *pData, int size)
{
    memcpy(m_Pcm+m_PcmSize, pData, size);
    m_PcmSize = m_PcmSize + size;
    int data_size = av_get_bytes_per_sample(m_PcmFormat);
    if (m_PcmSize <=data_size *1024 *m_PcmChannel)
    {
        return;
    }
    memcpy(m_PcmPointer[0], m_Pcm, data_size * 1024 * m_PcmChannel);

    m_PcmSize = m_PcmSize - data_size * 1024 * m_PcmChannel;
    memcpy(m_Pcm, m_Pcm + data_size * 1024 * m_PcmChannel, m_PcmSize);

    frame->pts = pts;
    pts += 1024;
    frame->nb_samples = 1024;
    frame->format = c->sample_fmt;
    frame->channel_layout = c->channel_layout;
    frame->sample_rate = c->sample_rate;
    if (av_frame_get_buffer(frame, 0)<0)
    {
        (stderr, "Could not allocate audio data buffers\n");
        return ;
    }

    int ret = 0;
    if (swr_convert(resample_context, frame->extended_data, frame->nb_samples, (const uint8_t**)m_PcmPointer, 1024)<0)
    {
        LOGE( "Could not convert input samples (error )\n");
        if (NULL != frame)
        {
            av_frame_unref(frame);
        }
        return ;
    }
    ret = avcodec_send_frame(c, frame);
    if (ret < 0) {
        LOGE("Error sending the frame to the encoder\n");
        if (NULL != frame)
        {
            av_frame_unref(frame);
        }
        return;
    }
    if (NULL != frame)
    {
        av_frame_unref(frame);
    }
}
bool Pcm2AAC::GetData(char *&pOutData, int &iSize)
{
    int  ret = avcodec_receive_packet(c, packet);
    if (ret <0)
    {
        return false;
    }
    AddADTS(packet->size+7);
    memcpy(m_pOutData+7, packet->data, packet->size);
    iSize = packet->size+7;
    pOutData = m_pOutData;
    av_packet_unref(packet);
    return true;
}
void Pcm2AAC::AddADTS(int packetLen)
{
    int profile = 1; // AAC LC
    int freqIdx = 0xb; // 44.1KHz
    int chanCfg = m_PcmChannel; // CPE

    if (96000 == m_PcmSampleRate)
    {
        freqIdx = 0x00;
    }
    else if(88200 == m_PcmSampleRate)
    {
        freqIdx = 0x01;
    }
    else if (64000 == m_PcmSampleRate)
    {
        freqIdx = 0x02;
    }
    else if (48000 == m_PcmSampleRate)
    {
        freqIdx = 0x03;
    }
    else if (44100 == m_PcmSampleRate)
    {
        freqIdx = 0x04;
    }
    else if (32000 == m_PcmSampleRate)
    {
        freqIdx = 0x05;
    }
    else if (24000 == m_PcmSampleRate)
    {
        freqIdx = 0x06;
    }
    else if (22050 == m_PcmSampleRate)
    {
        freqIdx = 0x07;
    }
    else if (16000 == m_PcmSampleRate)
    {
        freqIdx = 0x08;
    }
    else if (12000 == m_PcmSampleRate)
    {
        freqIdx = 0x09;
    }
    else if (11025 == m_PcmSampleRate)
    {
        freqIdx = 0x0a;
    }
    else if (8000 == m_PcmSampleRate)
    {
        freqIdx = 0x0b;
    }
    else if (7350 == m_PcmSampleRate)
    {
        freqIdx = 0xc;
    }
    // fill in ADTS data
    m_pOutData[0] = 0xFF;
    m_pOutData[1] = 0xF1;
    m_pOutData[2] = ((profile) << 6) + (freqIdx << 2) + (chanCfg >> 2);
    m_pOutData[3] = (((chanCfg & 3) << 6) + (packetLen >> 11));
    m_pOutData[4] = ((packetLen & 0x7FF) >> 3);
    m_pOutData[5] = (((packetLen & 7) << 5) + 0x1F);
    m_pOutData[6] = 0xFC;
}
