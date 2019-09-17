package com.vidyo.works.support.utils;

import android.content.Context;
import android.content.res.Configuration;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import com.vidyo.works.support.JniBridge;

import static com.vidyo.LmiDeviceManager.LmiVideoCapturerInternal.ORIENTATION_DOWN;
import static com.vidyo.LmiDeviceManager.LmiVideoCapturerInternal.ORIENTATION_LEFT;
import static com.vidyo.LmiDeviceManager.LmiVideoCapturerInternal.ORIENTATION_RIGHT;
import static com.vidyo.LmiDeviceManager.LmiVideoCapturerInternal.ORIENTATION_UP;

public class OrientationManager implements SensorEventListener {

    private Context context;
    private JniBridge jniBridge;

    /* ORIENTATION */
    private int currentOrientation;
    private SensorManager sensorManager;

    private static final float degreePerRadian = (float) (180.0f / Math.PI);

    private float[] mGData = new float[3];
    private float[] mMData = new float[3];
    private float[] mR = new float[16];
    private float[] mI = new float[16];
    private float[] mOrientation = new float[3];
    /* END */

    public OrientationManager(Context context, JniBridge jniBridge) {
        this.context = context;
        this.jniBridge = jniBridge;

        /* ROTATION */
        currentOrientation = -1;
        sensorManager = (SensorManager) context.getSystemService(Context.SENSOR_SERVICE);
        Sensor gSensor = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        Sensor mSensor = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
        sensorManager.registerListener(this, gSensor, SensorManager.SENSOR_DELAY_FASTEST);
        sensorManager.registerListener(this, mSensor, SensorManager.SENSOR_DELAY_FASTEST);
    }

    public void release() {
        if (sensorManager != null) {
            Sensor accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
            Sensor magnetic = sensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);

            sensorManager.unregisterListener(this, accelerometer);
            sensorManager.unregisterListener(this, magnetic);
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        int newOrientation = currentOrientation;

        int type = event.sensor.getType();
        float[] data;
        if (type == Sensor.TYPE_ACCELEROMETER) {
            data = mGData; /* set accelerometer data pointer */
        } else if (type == Sensor.TYPE_MAGNETIC_FIELD) {
            data = mMData; /* set magnetic data pointer */
        } else {
            return;
        }

        /* copy the data to the appropriate array */
        /* copy the data to the appropriate array */
        System.arraycopy(event.values, 0, data, 0, 3);

        /*
         * calculate the rotation data from the latest accelerometer and
         * magnetic data
         */
        boolean ret = SensorManager.getRotationMatrix(mR, mI, mGData, mMData);
        if (!ret) return;

        SensorManager.getOrientation(mR, mOrientation);

        Configuration config = this.context.getResources().getConfiguration();
        boolean hardKeyboardOrientFix = (config.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO);

        int pitch = (int) (mOrientation[1] * degreePerRadian);
        int roll = (int) (mOrientation[2] * degreePerRadian);

        if (pitch < -45) {
            if (hardKeyboardOrientFix)
                newOrientation = ORIENTATION_LEFT;
            else
                newOrientation = ORIENTATION_UP;
        } else if (pitch > 45) {
            if (hardKeyboardOrientFix)
                newOrientation = ORIENTATION_RIGHT;
            else
                newOrientation = ORIENTATION_DOWN;
        } else if (roll < -45 && roll > -135) {
            if (hardKeyboardOrientFix)
                newOrientation = ORIENTATION_UP;
            else
                newOrientation = ORIENTATION_RIGHT;
        } else if (roll > 45 && roll < 135) {
            if (hardKeyboardOrientFix)
                newOrientation = ORIENTATION_DOWN;
            else
                newOrientation = ORIENTATION_LEFT;
        }

        if (newOrientation != currentOrientation) {
            currentOrientation = newOrientation;
            if (jniBridge != null) jniBridge.SetOrientation(newOrientation);
        }
    }
}