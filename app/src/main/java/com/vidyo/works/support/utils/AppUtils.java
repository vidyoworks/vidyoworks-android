package com.vidyo.works.support.utils;

import android.Manifest;
import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.PackageManager;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.provider.Settings;
import android.telephony.TelephonyManager;
import android.util.Log;

import com.vidyo.works.support.JniBridge;
import com.vidyo.works.support.R;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class AppUtils {

    private static final String TAG = "AppUtils";

    public static final int BACK_CAMERA = 0;
    public static final int REAR_CAMERA = 1;

    private static final String CERTIFICATE_RAW_NAME = "ca-certificates.crt";

    @SuppressLint("HardwareIds")
    public static String getUniqueID(Context context) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && context.checkSelfPermission(Manifest.permission.READ_PHONE_STATE) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "No permissions granted for Telephony manager");
            return "";
        }

        String machineID = null;

        TelephonyManager tManager = (TelephonyManager) context.getSystemService(Context.TELEPHONY_SERVICE);
        if (tManager != null) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                machineID = tManager.getImei();
            } else {
                machineID = tManager.getDeviceId();
            }
        }

        if (machineID == null) {
            machineID = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.ANDROID_ID);
        }

        if (machineID == null) return "";

        return machineID;
    }

    public static String writeCaCertificates(Context context) {
        try {
            InputStream caCertStream = context.getResources().openRawResource(R.raw.ca_certificates);
            File caCertDirectory;

            String pathDir = getAndroidInternalMemDir(context);
            if (pathDir == null) return null;

            caCertDirectory = new File(pathDir);

            File caFile = new File(caCertDirectory, CERTIFICATE_RAW_NAME);
            FileOutputStream caCertFile = new FileOutputStream(caFile);

            byte[] buffer = new byte[1024];
            int len;

            while ((len = caCertStream.read(buffer)) != -1) {
                caCertFile.write(buffer, 0, len);
            }

            caCertStream.close();
            caCertFile.close();
            return caFile.getPath();
        } catch (Exception e) {
            return null;
        }
    }

    public static void configDestiny(Context context, JniBridge jniBridge) {
        double density = context.getResources().getDisplayMetrics().density;
        jniBridge.SetPixelDensity(density);
    }

    public static void configAudioManager(Context context) {
        AudioManager audioManager = (AudioManager) context.getSystemService(Context.AUDIO_SERVICE);
        if (audioManager != null) {
            audioManager.setSpeakerphoneOn(true);
        }
    }

    public static String getAndroidInternalMemDir(Context context) {
        File fileDir = context.getFilesDir();
        return fileDir != null ? fileDir.toString() + File.separator : null;
    }

    public static String getAndroidCacheDir(Context context) {
        File cacheDir = context.getCacheDir();
        return cacheDir != null ? cacheDir.toString() + File.separator : null;
    }

    public static boolean isNetworkAvailable(Context context) {
        if (context == null) return false;

        ConnectivityManager connectivityManager = (ConnectivityManager) context.getSystemService(Context.CONNECTIVITY_SERVICE);
        if (connectivityManager == null) return false;

        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        return activeNetworkInfo != null && activeNetworkInfo.isConnected();
    }
}