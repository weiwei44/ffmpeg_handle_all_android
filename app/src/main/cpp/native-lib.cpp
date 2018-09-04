#include <jni.h>
#include <string>
#include "AndroidLog.h"

extern "C"{
#include <libavcodec/jni.h>
}

#include "encoder_audio.h"
#include "demux_media.h"
#include "decoder_media.h"

DemuxMedia* demuxMedia= NULL;

extern "C"
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *res) {
    av_jni_set_java_vm(vm, 0);
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_mobile_weiwei_utils_FFmpegUtils_demuxMedia(JNIEnv *env, jobject instance, jstring path_,
                                                      jstring dstPath_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    const char *dstPath = env->GetStringUTFChars(dstPath_, 0);

    if(demuxMedia == NULL){
        demuxMedia = new DemuxMedia();
    }
    demuxMedia->init(path,dstPath);
    int ret = demuxMedia->openInput();
    if(ret >= 0){
        ret = demuxMedia->openOutput();
    }
    if(ret < 0){
        demuxMedia->closeInput();
        demuxMedia->closeOutput();
    }

    while (true){
        AVPacket* packet = demuxMedia->readPacketFromSource();
        if(packet){
            ret = demuxMedia->writePacket(packet);
            if(ret >= 0){
                av_packet_unref(packet);
                LOGE("writePacket Success!");
            }else{
                LOGE("writePacket failed!");
            }
        } else{
            LOGE("结束");
            av_packet_free(&packet);
            demuxMedia->closeInput();
            demuxMedia->closeOutput();
            break;
        }
    }

    env->ReleaseStringUTFChars(path_, path);
    env->ReleaseStringUTFChars(dstPath_, dstPath);
}extern "C"
JNIEXPORT void JNICALL
Java_com_mobile_weiwei_utils_FFmpegUtils_encoderAudio(JNIEnv *env, jobject instance, jstring path_,
                                                      jstring dstPath_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    const char *dstPath = env->GetStringUTFChars(dstPath_, 0);

    Pcm2AAC  pcm2AAC;
    if (!pcm2AAC.Init(44100, AV_SAMPLE_FMT_S16, 2))
    {
        LOGE("pcm2AAC Init error\r\n");
        return;
    }
    LOGE("pasing start\r\n");
    char frame_buf[1024] = { 0 };
    //输出文件
    FILE * OutFile = fopen(dstPath, "wb");
    //读取文件
    FILE * InFile = fopen(path, "rb");
    char * pOutData = NULL;
    int OutSize = 0;
    while (true)
    {
        int iReadSize = fread(frame_buf, 1, 512, InFile);
        if (iReadSize <= 0)
        {
            break;
        }
        pcm2AAC.AddData(frame_buf, iReadSize);

        while (true)
        {
            if (!pcm2AAC.GetData(pOutData, OutSize))
            {
                break;
            }
            fwrite(pOutData, 1, OutSize, OutFile);
        }
    }
    LOGE("pasing end\r\n");
    fclose(OutFile);
    fclose(InFile);


    env->ReleaseStringUTFChars(path_, path);
    env->ReleaseStringUTFChars(dstPath_, dstPath);
}extern "C"
JNIEXPORT void JNICALL
Java_com_mobile_weiwei_utils_FFmpegUtils_cutFile(JNIEnv *env, jobject instance, jstring path_,
                                                 jstring dstPath_, jint startTime, jint cutTime) {
    const char *path = env->GetStringUTFChars(path_, 0);
    const char *dstPath = env->GetStringUTFChars(dstPath_, 0);

    if(demuxMedia == NULL){
        demuxMedia = new DemuxMedia();
    }
    demuxMedia->init(path,dstPath);
    int ret = demuxMedia->openInput();
    if(ret >= 0){
        ret = demuxMedia->openOutput();
    }
    if(ret < 0){
        demuxMedia->closeInput();
        demuxMedia->closeOutput();
    }

    int packetCount = 0;
    int64_t lastPacketPts  = AV_NOPTS_VALUE;
    int64_t lastPts  = AV_NOPTS_VALUE;

    while (true){
        AVPacket* packet = demuxMedia->readPacketFromSource();
        if(packet){
            packetCount++;
            LOGE("pts = %lld,packetCount = %d",packet->pts,packetCount);
            if(packetCount <= startTime || packetCount >= startTime + cutTime){
                if(packetCount >= startTime + cutTime){
                    if(packet->pts - lastPacketPts > 120){
                        lastPts = lastPacketPts ;
                    }else{
                        auto diff = packet->pts - lastPacketPts;
                        lastPts += diff;
                    }
                }
                lastPacketPts = packet->pts;
                if(lastPacketPts != AV_NOPTS_VALUE){
                    packet->pts = packet->dts = lastPts;
                }
                ret = demuxMedia->writePacket(packet);

                if(ret >= 0){
                    av_packet_unref(packet);
                    LOGE("writePacket Success!");
                }else{
                    LOGE("writePacket failed!");
                }
            }

        } else{
            LOGE("结束");
            av_packet_free(&packet);
            demuxMedia->closeInput();
            demuxMedia->closeOutput();
            break;
        }
    }

    env->ReleaseStringUTFChars(path_, path);
    env->ReleaseStringUTFChars(dstPath_, dstPath);
}extern "C"
JNIEXPORT void JNICALL
Java_com_mobile_weiwei_utils_FFmpegUtils_testdemuxMedia(JNIEnv *env, jobject instance,
                                                        jstring path_, jstring dstPath_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    const char *dstPath = env->GetStringUTFChars(dstPath_, 0);

    // TODO
    if(demuxMedia == NULL){
        demuxMedia = new DemuxMedia();
    }
    demuxMedia->init(path,dstPath);
    demuxMedia->ffmpegExp();

    env->ReleaseStringUTFChars(path_, path);
    env->ReleaseStringUTFChars(dstPath_, dstPath);
}extern "C"
JNIEXPORT void JNICALL
Java_com_mobile_weiwei_utils_FFmpegUtils_testdecodeMedia(JNIEnv *env, jobject instance,
                                                         jstring path_, jstring dstPath_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    const char *dstPath = env->GetStringUTFChars(dstPath_, 0);

    // TODO
    Decoder* decoder = new Decoder();
    decoder->init(path,dstPath);
    decoder->ffmpegExp();

    env->ReleaseStringUTFChars(path_, path);
    env->ReleaseStringUTFChars(dstPath_, dstPath);
}