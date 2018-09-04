package com.mobile.weiwei;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Environment;
import android.support.annotation.RequiresApi;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.webkit.WebView;
import android.widget.Button;
import android.widget.TextView;

import com.mobile.weiwei.utils.FFmpegUtils;
import com.tbruyelle.rxpermissions2.RxPermissions;

import io.reactivex.functions.Consumer;

public class MainActivity extends AppCompatActivity {

    private FFmpegUtils utils = null;
    private String path = "/storage/emulated/0/weiwei/play.mp4";
    private String rtmpPath = "rtmp://live.hkstv.hk.lxdns.com/live/hks";
    private String udpPath = "udp://127.0.0.1:1234";
    private String localRtmpPath = "rtmp://localhost:1935";

    private String outPath = "/storage/emulated/0/weiwei/test3.ts";

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        requestPermission();

        Button bt_decoder = findViewById(R.id.bt_decoder);
        bt_decoder.setOnClickListener(v->
                //new Thread(() -> utils.demuxMedia(rtmpPath, udpPath)).start()
               // new Thread(() -> utils.testdemuxMedia(path, "/storage/emulated/0/weiwei/ffmpegplay_cut.mp4")).start()
                new Thread(() -> utils.testdemuxMedia("/storage/emulated/0/weiwei/2.mp3", "/storage/emulated/0/weiwei/3.mp3")).start()
            );

        Button bt_encoder = findViewById(R.id.bt_encoder);
        bt_encoder.setOnClickListener(v->
                new Thread(() -> utils.encoderAudio("/storage/emulated/0/weiwei/tdjm.pcm", "/storage/emulated/0/weiwei/tdjm1.aac")).start()
        );

        Button bt_cut = findViewById(R.id.bt_cut);
        bt_cut.setOnClickListener(v->
                new Thread(() -> utils.cutFile("/storage/emulated/0/weiwei/play.mp4", "/storage/emulated/0/weiwei/play11.mp4",200,100)).start()
        );

        Button bt_decoder_ff = findViewById(R.id.bt_decoder_ff);
        bt_decoder_ff.setOnClickListener(v->
                //new Thread(() -> utils.testdecodeMedia("/storage/emulated/0/weiwei/play.mp4", "/storage/emulated/0/weiwei/ffmpegplay.pcm")).start()
                new Thread(() -> utils.testdecodeMedia("/storage/emulated/0/weiwei/test.mp4", "/storage/emulated/0/weiwei/ffmpegplay.yuv")).start()

        );

    }

    @SuppressLint("CheckResult")
    @RequiresApi(api = Build.VERSION_CODES.JELLY_BEAN)
    private void requestPermission(){
        new RxPermissions(this)
                .request(Manifest.permission.WRITE_EXTERNAL_STORAGE,
                        Manifest.permission.READ_EXTERNAL_STORAGE)
                .subscribe(aBoolean -> {
                    if(aBoolean){
                        utils = new FFmpegUtils();
                    }
                });
    }
}
