package com.vidyo.works.support.activity;

import android.Manifest;
import android.annotation.TargetApi;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.widget.Toast;

import com.vidyo.works.support.R;

import static android.Manifest.permission.CAMERA;
import static android.Manifest.permission.RECORD_AUDIO;
import static android.Manifest.permission.WRITE_EXTERNAL_STORAGE;

public class HomeActivity extends AppCompatActivity {

    private static final String[] PERMISSIONS_LIST = new String[]{
            Manifest.permission.CAMERA, // Access device camera
            Manifest.permission.RECORD_AUDIO, // Audio access for Voip processing
            Manifest.permission.WRITE_EXTERNAL_STORAGE, // Write log file outside of internal app storage
            Manifest.permission.READ_PHONE_STATE // Required for unique device id from TelephonyManager
    };

    private static final int PERMISSIONS_RC = 0x85ba;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_home);

        if (requestPermissions()) {
            granted();
        }
    }

    public void portalLogin(View view) {
        startActivity(new Intent(this, LoginActivity.class));
    }

    public void guestJoin(View view) {
        startActivity(new Intent(this, GuestJoinActivity.class));
    }

    /**
     * Callback received when a permissions request has been completed.
     */
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSIONS_RC) {
            if (grantResults.length == 4
                    && grantResults[0] == PackageManager.PERMISSION_GRANTED
                    && grantResults[1] == PackageManager.PERMISSION_GRANTED
                    && grantResults[2] == PackageManager.PERMISSION_GRANTED
                    && grantResults[3] == PackageManager.PERMISSION_GRANTED) {
                // Granted
                granted();
            } else {
                // Go over again.
                requestPermissions();
            }
        }
    }

    private void granted() {
        Toast.makeText(this, "All has been granted.", Toast.LENGTH_SHORT).show();
    }

    private boolean requestPermissions() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return true;
        }

        if (checkSelfPermission(WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(CAMERA) == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }

        if (shouldShowRequestPermissionRationale(WRITE_EXTERNAL_STORAGE) && shouldShowRequestPermissionRationale(CAMERA)
                && shouldShowRequestPermissionRationale(RECORD_AUDIO)) {
            Snackbar.make(findViewById(R.id.main_content), R.string.permission_rationale, Snackbar.LENGTH_INDEFINITE)
                    .setAction(android.R.string.ok, new View.OnClickListener() {
                        @Override
                        @TargetApi(Build.VERSION_CODES.M)
                        public void onClick(View v) {
                            requestPermissions(PERMISSIONS_LIST, PERMISSIONS_RC);
                        }
                    });
        } else {
            requestPermissions(PERMISSIONS_LIST, PERMISSIONS_RC);
        }

        return false;
    }
}