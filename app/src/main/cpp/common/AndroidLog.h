//
// Created by BMW on 2018/7/17.
//

#ifndef LOG_H
#define LOG_H

#define LOG_TAG "weiwei"

#ifdef ANDROID
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#else
#include <stdio.h>

#define LOGD(...) printf(LOG_TAG, __VA_ARGS__)
#define LOGI(...) printf(LOG_TAG, __VA_ARGS__)
#define LOGE(...) printf(LOG_TAG, __VA_ARGS__)
#endif

#endif
