package com.mobile.weiwei;

import android.Manifest;
import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.os.Build;
import android.os.Environment;
import android.support.annotation.RequiresApi;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
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

    private String outPath = "/storage/emulated/0/weiwei/test3.ts";

    @TargetApi(Build.VERSION_CODES.JELLY_BEAN)
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        requestPermission();

        Button bt_decoder = findViewById(R.id.bt_decoder);
        bt_decoder.setOnClickListener(v->
                new Thread(() -> utils.demuxMedia(rtmpPath, udpPath)).start()
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
