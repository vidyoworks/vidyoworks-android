package com.vidyo.works.support.activity;

import android.os.Bundle;
import android.support.design.widget.Snackbar;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;
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

import java.util.Random;

/**
 * A Guest join screen that offers join via portal + roomKey + display name.
 */
public class GuestJoinActivity extends AppCompatActivity implements LmiDeviceManagerView.Callback {

    private static final String TAG = "GuestJoin";

    private static final String PORTAL = "https://<VIDOY_PORTAL_PLACEHOLDER>.com";
    private static final String KEY = "<ROOM_KEY_PLACEHOLDER>";
    private static final String NAME = "Guest Mobile";

    // UI references.
    private EditText portal;
    private EditText key;
    private EditText user;

    private View joinForm;

    private LinearLayout controlForm;

    private JniBridge jniBridge;

    private OrientationManager orientationManager;

    private boolean doRender = false;
    private boolean callStarted = false;

    private LmiDeviceManagerView lmiDeviceManagerView;

    private ProgressBar joinProgress;

    private Button joinButton;
    private Button cancelButton;

    private long joinCountDown = 0L;
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
        setContentView(R.layout.activity_guest);

        EventBus.getDefault().register(this);

        // Set up the login form.
        portal = findViewById(R.id.portal_field);
        key = findViewById(R.id.room_key);
        user = findViewById(R.id.user_name);

        portal.setText(PORTAL);
        key.setText(KEY);
        user.setText(NAME);

        AppUtils.configAudioManager(this);

        user.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int id, KeyEvent keyEvent) {
                if (id == EditorInfo.IME_ACTION_DONE || id == EditorInfo.IME_NULL) {
                    attemptJoin();
                    return true;
                }

                return false;
            }
        });

        joinForm = findViewById(R.id.join_params_frame);

        joinProgress = findViewById(R.id.join_progress);
        joinProgress.setVisibility(View.GONE);

        joinButton = findViewById(R.id.join_room_btn);
        joinButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                attemptJoin();
            }
        });

        cancelButton = findViewById(R.id.cancel_join);
        cancelButton.setEnabled(false);
        cancelButton.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View view) {
                callEnded();
            }
        });

        initializeLibrary();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        EventBus.getDefault().unregister(this);

        stopDevices();

        if (orientationManager != null)
            orientationManager.release();

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
            // jniBridge.Configure();

            ViewGroup frame = findViewById(R.id.main_content);

            controlForm = new LinearLayout(this);
            controlForm.setOrientation(LinearLayout.HORIZONTAL);

            lmiDeviceManagerView = new LmiDeviceManagerView(this, this);
            lmiDeviceManagerView.setVisibility(View.GONE);

            frame.addView(lmiDeviceManagerView);
            frame.addView(controlForm);

            addEndCallView(controlForm);
            addActionViewView(controlForm);
            addSendChatMessageView(controlForm);

            controlForm.setVisibility(View.GONE);
        }
    }

    private void attemptJoin() {
        // Reset errors.
        portal.setError(null);
        key.setError(null);
        user.setError(null);

        // Store values at the time of the login attempt.
        String portalParam = portal.getText().toString();
        String keyParam = key.getText().toString();
        String userParam = user.getText().toString();

        joinProgress.setVisibility(View.VISIBLE);
        joinButton.setEnabled(false);
        cancelButton.setEnabled(true);

        jniBridge.JoinRoomLink(portalParam, keyParam, userParam, "");

        jniBridge.HideToolBar(true);
        jniBridge.SetEchoCancellation(true);

        joinCountDown = System.currentTimeMillis();
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
            case MESSAGE:
                String message = (String) event.getValue();
                Log.i(TAG, "Message: " + message);
                break;
            case CALLING:
                break;
            case STARTED:
                AppUtils.configDestiny(this, jniBridge);

                long time = (System.currentTimeMillis() - joinCountDown) / 1000;
                Snackbar.make(user, "Started. Join takes: " + time + " sec.", Snackbar.LENGTH_SHORT).show();

                callStarted();
                break;
            case ENDED:
                Snackbar.make(user, "Ended", Snackbar.LENGTH_SHORT).show();

                callEnded();
                break;
            case INIT_PASS:
                Snackbar.make(user, "Library has been initialized.", Snackbar.LENGTH_SHORT).show();
                break;
            case ERROR:
                callEnded();

                Toast.makeText(this, "Join error", Toast.LENGTH_SHORT).show();
                break;
            case LOGOUT:
                callEnded();

                if (AppUtils.isNetworkAvailable(this)) {
                    Toast.makeText(this, "Unexpected logout", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "No network available", Toast.LENGTH_SHORT).show();
                }
                break;
            case GROUP_CHAT_MESSAGE:
                String displayName = (String) event.getValues()[0];
                String chatMessage = (String) event.getValues()[1];

                Toast.makeText(this, "User: " + displayName + "; Message: " + chatMessage, Toast.LENGTH_SHORT).show();
                break;
        }
    }

    private void callStarted() {
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        callStarted = true;
        joinProgress.setVisibility(View.GONE);
        joinButton.setEnabled(true);

        joinForm.setVisibility(View.GONE);
        lmiDeviceManagerView.setVisibility(View.VISIBLE);

        controlForm.setVisibility(View.VISIBLE);

        if (jniBridge != null) {
            jniBridge.SetCameraDevice(AppUtils.REAR_CAMERA);
            jniBridge.StartConferenceMedia();
            jniBridge.EnterForeground();
        }

        startDevices();
    }

    private void callEnded() {
        getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        callStarted = false;

        joinButton.setEnabled(true);
        cancelButton.setEnabled(false);

        joinProgress.setVisibility(View.GONE);

        joinForm.setVisibility(View.VISIBLE);
        lmiDeviceManagerView.setVisibility(View.GONE);

        controlForm.setVisibility(View.GONE);

        stopDevices();

        if (jniBridge != null) {
            jniBridge.RenderRelease();

            jniBridge.LeaveConference();
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

    private void addSendChatMessageView(ViewGroup frame) {
        Button sendMessage = new Button(this);
        sendMessage.setText(R.string.send_group_chat_message);
        sendMessage.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                if (jniBridge != null) {
                    int x = new Random().nextInt(100);
                    String message = "hello~" + x;

                    jniBridge.SendGroupChatMessage(message);
                }
            }
        });

        frame.addView(sendMessage);

        sendMessage.getLayoutParams().width = ViewGroup.LayoutParams.WRAP_CONTENT;
        sendMessage.getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    }
}