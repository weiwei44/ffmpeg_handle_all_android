#include <jni.h>
#include <string>
#include "AndroidLog.h"

extern "C"{
#include <libavcodec/jni.h>
}

#include "demux_media.h"
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
}