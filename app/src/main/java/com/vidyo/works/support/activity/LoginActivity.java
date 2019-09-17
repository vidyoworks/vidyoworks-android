package com.vidyo.works.support.activity;

import android.os.Bundle;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.Toast;

import com.vidyo.LmiDeviceManager.LmiDeviceManagerView;
import com.vidyo.LmiDeviceManager.LmiVideoCapturer;
import com.vidyo.works.support.JniBridge;
import com.vidyo.works.support.R;
import com.vidyo.works.support.event.HomeBus;
import com.vidyo.works.support.utils.AppUtils;
import com.vidyo.works.support.utils.OrientationManager;

import org.greenrobot.eventbus.EventBus;
import org.greenrobot.eventbus.Subscribe;
import org.greenrobot.eventbus.ThreadMode;

/**
 * A login screen that offers login via email/password into desired portal tenant.
 * After successful login you can join into the personal room.
 */
public class LoginActivity extends AppCompatActivity implements LmiDeviceManagerView.Callback {

    private static final String PORTAL = "https://<VIDOY_PORTAL_PLACEHOLDER>.com";
    private static final String USER = "<USER_NAME_PLACEHOLDER>";
    private static final String PASSWORD = "<PASSWORD_PLACEHOLDER>";

    // UI references.
    private EditText portal;
    private EditText user;
    private EditText password;

    private View loginPanelForm;
    private View loginForm;
    private View logoutForm;

    private LinearLayout controlForm;

    private JniBridge jniBridge;

    private OrientationManager orientationManager;

    private boolean doRender = false;
    private boolean callStarted = false;

    private LmiDeviceManagerView lmiDeviceManagerView;

    private ProgressBar loginProgress;

    private Button loginButton;

    private long loginCountDown = 0L;

    private boolean logInStatus = false;
    private String entityId;

    private boolean showStatistic = false;

    @Override
    protected void onResume() {
        super.onResume();

        if (callStarted) {
            resumeCall();
            LmiVideoCapturer.onActivityResume();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        if (callStarted) {
            pauseCall();
            LmiVideoCapturer.onActivityPause();
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_login);

        EventBus.getDefault().register(this);

        AppUtils.configAudioManager(this);

        // Set up the login form.
        portal = findViewById(R.id.portal_field);
        user = findViewById(R.id.user_name);
        password = findViewById(R.id.password);

        portal.setText(PORTAL);
        user.setText(USER);
        password.setText(PASSWORD);

        loginPanelForm = findViewById(R.id.login_params_frame);

        loginForm = findViewById(R.id.login_frame);
        logoutForm = findViewById(R.id.logout_form);

        loginProgress = findViewById(R.id.login_progress);
        loginProgress.setVisibility(View.GONE);

        loginButton = findViewById(R.id.login_btn);
        loginButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                attemptLogin();
            }
        });

        findViewById(R.id.logout_btn).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                logoutAttempt();
            }
        });


        findViewById(R.id.login_join).setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                attemptJoinRoom();
            }
        });

        updateUI();

        initializeLibrary();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);

        stopDevices();

        if (orientationManager != null) orientationManager.release();

        if (jniBridge != null) {
            jniBridge.uninitialize();
            jniBridge = null;
        }
    }

    private void initializeLibrary() {
        jniBridge = new JniBridge();
        orientationManager = new OrientationManager(this, jniBridge);

        boolean result = jniBridge.initialize(
                AppUtils.writeCaCertificates(this),
                AppUtils.getAndroidCacheDir(this),
                AppUtils.getAndroidInternalMemDir(this),
                this);

        if (result) {
            ViewGroup frame = findViewById(R.id.main_content);

            controlForm = new LinearLayout(this);
            controlForm.setOrientation(LinearLayout.HORIZONTAL);

            lmiDeviceManagerView = new LmiDeviceManagerView(this, this);
            lmiDeviceManagerView.setVisibility(View.GONE);

            frame.addView(lmiDeviceManagerView);
            frame.addView(controlForm);

            addEndCallView(controlForm);
            addActionViewView(controlForm);

            controlForm.setVisibility(View.GONE);
        }
    }

    @Override
    public void LmiDeviceManagerViewRender() {
        if (doRender && jniBridge != null) {
            jniBridge.Render();
        }
    }

    @Override
    public void LmiDeviceManagerViewResize(int width, int height) {
        if (jniBridge != null) jniBridge.Resize(width, height);
    }

    @Override
    public void LmiDeviceManagerViewRenderRelease() {
        if (jniBridge != null) jniBridge.RenderRelease();
    }

    @Override
    public void LmiDeviceManagerViewTouchEvent(int id, int type, int x, int y) {
        if (jniBridge != null) jniBridge.TouchEvent(id, type, x, y);
    }

    @Override
    public void onWindowFocusChanged(final boolean hasWindowFocus) {
        super.onWindowFocusChanged(hasWindowFocus);

        if (hasWindowFocus) {
            resumeCall();
        }
    }

    @Subscribe(threadMode = ThreadMode.MAIN)
    public void onEvent(HomeBus event) {
        switch (event.getCall()) {
            case INIT_PASS:
                Snackbar.make(user, "Library has been initialized.", Snackbar.LENGTH_SHORT).show();
                break;
            case LOGIN:
                entityId = (String) event.getValue();

                long time = (System.currentTimeMillis() - loginCountDown) / 1000;
                Snackbar.make(user, "Log-in success. Take: " + time + " sec.", Snackbar.LENGTH_SHORT).show();

                logInStatus = true;

                updateUI();
                break;
            case LOGOUT:
                Snackbar.make(user, "Log out success", Snackbar.LENGTH_SHORT).show();

                logInStatus = false;

                updateUI();
                break;
            case MESSAGE:
                Snackbar.make(user, (String) event.getValue(), Snackbar.LENGTH_LONG).show();
                break;
            case CALLING:
                break;
            case STARTED:
                AppUtils.configDestiny(this, jniBridge);

                callStarted();
                break;
            case ENDED:
                Snackbar.make(user, "Ended", Snackbar.LENGTH_SHORT).show();

                callEnded();
                break;
            case ERROR:
                callEnded();

                Toast.makeText(this, "Join error", Toast.LENGTH_SHORT).show();
                break;
        }
    }

    private void updateUI() {
        loginProgress.setVisibility(View.GONE);
        loginButton.setEnabled(!logInStatus);

        loginForm.setVisibility(logInStatus ? View.GONE : View.VISIBLE);
        logoutForm.setVisibility(!logInStatus ? View.GONE : View.VISIBLE);
    }

    private void callStarted() {
        callStarted = true;
        loginProgress.setVisibility(View.GONE);
        loginButton.setEnabled(true);

        loginPanelForm.setVisibility(View.GONE);
        lmiDeviceManagerView.setVisibility(View.VISIBLE);

        controlForm.setVisibility(View.VISIBLE);

        if (jniBridge != null) {
            jniBridge.SetCameraDevice(0);
            jniBridge.StartConferenceMedia();
            jniBridge.EnterForeground();
        }

        startDevices();
    }

    private void callEnded() {
        callStarted = false;

        loginButton.setEnabled(true);

        loginProgress.setVisibility(View.GONE);

        loginPanelForm.setVisibility(View.VISIBLE);
        lmiDeviceManagerView.setVisibility(View.GONE);

        controlForm.setVisibility(View.GONE);

        stopDevices();

        if (jniBridge != null) {
            jniBridge.RenderRelease();
        }
    }

    void startDevices() {
        doRender = true;
    }

    void stopDevices() {
        doRender = false;
    }

    private void resumeCall() {
        lmiDeviceManagerView.onResume();
        jniBridge.EnterForeground();
    }

    private void pauseCall() {
        lmiDeviceManagerView.onPause();
        jniBridge.EnterBackground();
    }

    private void logoutAttempt() {
        loginProgress.setVisibility(View.VISIBLE);

        jniBridge.Logout();
    }

    private void attemptJoinRoom() {
        loginProgress.setVisibility(View.VISIBLE);

        if (jniBridge != null) {
            jniBridge.PortalServiceJoinConference(entityId);
        }
    }

    private void attemptLogin() {
        portal.setError(null);
        user.setError(null);
        password.setError(null);

        // Store values at the time of the login attempt.
        String portalParam = portal.getText().toString();
        String userParam = user.getText().toString();
        String passwordParam = password.getText().toString();

        loginProgress.setVisibility(View.VISIBLE);
        loginButton.setEnabled(false);

        jniBridge.Login(portalParam, userParam, passwordParam);

        jniBridge.HideToolBar(true);
        jniBridge.SetEchoCancellation(true);

        loginCountDown = System.currentTimeMillis();
    }

    private void addActionViewView(ViewGroup frame) {
        Button changeStatView = new Button(this);
        changeStatView.setText(R.string.toggle_stats);

        changeStatView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (jniBridge != null) {
                    showStatistic = !showStatistic;
                    jniBridge.setStatsVisibility(showStatistic);
                }
            }
        });

        frame.addView(changeStatView);

        changeStatView.getLayoutParams().width = ViewGroup.LayoutParams.WRAP_CONTENT;
        changeStatView.getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    }

    private void addEndCallView(ViewGroup frame) {
        Button endCall = new Button(this);
        endCall.setText(R.string.end_call);
        endCall.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (jniBridge != null) {
                    jniBridge.LeaveConference();
                }
            }
        });

        frame.addView(endCall);

        endCall.getLayoutParams().width = ViewGroup.LayoutParams.WRAP_CONTENT;
        endCall.getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    }
}