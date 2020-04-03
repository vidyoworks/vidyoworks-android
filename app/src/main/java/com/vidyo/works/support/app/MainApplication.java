package com.vidyo.works.support.app;

import android.app.Application;

import com.vidyo.works.support.audio.BluetoothManager;

public class MainApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        BluetoothManager.initBluetoothManager(getApplicationContext());
    }
}