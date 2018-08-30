package com.mobile.weiwei.utils;

public class FFmpegUtils {
    static {
        System.loadLibrary("native-lib");
    }


    /**
     * mp4转ts裸流，或者保存网络流到ts文件
     * 也可以直接进行流转发
     * @param path
     * @param dstPath
     */
    public native void demuxMedia(String path,String dstPath);


}
