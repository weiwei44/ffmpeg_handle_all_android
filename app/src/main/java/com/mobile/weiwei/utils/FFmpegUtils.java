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
    public native void testdemuxMedia(String path,String dstPath);

    /**
     * pcm to acc
     * @param path
     * @param dstPath
     */
    public native void encoderAudio(String path,String dstPath);

    /**
     *
     * @param path
     * @param dstPath
     * @param startTime 从第几秒开始剪切
     * @param cutTime  剪切多少秒
     */
    public native void cutFile(String path,String dstPath,int startTime,int cutTime);

    /**
     * 剪切音视频
     * @param path
     * @param dstPath
     */
    public native void testdecodeMedia(String path,String dstPath);

    /**
     * 获取图片
     * @param path
     * @param dstPath
     */
    public native void getPic(String path,String dstPath);

}
