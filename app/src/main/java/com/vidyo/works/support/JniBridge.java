package com.vidyo.works.support;

import android.app.Activity;
import android.util.Log;

import androidx.annotation.Keep;

import com.vidyo.works.support.event.HomeBus;
import com.vidyo.works.support.utils.AppUtils;

import org.greenrobot.eventbus.EventBus;

@Keep
public class JniBridge {

    static {
        System.loadLibrary("VidyoClientApp");
        System.loadLibrary("ndkVidyoSample");
    }

    private static final String TAG = "JNI_Bridge_Java";

    public JniBridge() {
    }

    public boolean initialize(Activity activity) {
        String caFileName = AppUtils.writeCaCertificates(activity);
        String logDir = AppUtils.getAndroidCacheDir(activity);
        String pathDir = AppUtils.getAndroidInternalMemDir(activity);

        Log.i(TAG, "caFileName --> " + caFileName);
        Log.i(TAG, "logDir --> " + logDir);
        Log.i(TAG, "pathDir --> " + pathDir);

        Log.i(TAG, "activity --> " + activity.getClass().toString());

        String logLevel = "FATAL ERROR WARNING INFO@AppGui INFO@AppEmcpClient INFO@LmiApp INFO@App INFO@AppGuiUser INFO@AppWebProxy";

        String uniqueId = AppUtils.getUniqueID(activity);
        Log.e(TAG, "Machine ID: " + uniqueId);

        long address = Construct(uniqueId, caFileName, logDir, pathDir, logLevel, activity);
        return address != 0;
    }

    public void uninitialize() {
        Dispose();
    }

    @Keep
    public void cameraSwitchCallback(String name) {
        Log.i(TAG, "cameraSwitchCallback received!");
    }

    @Keep
    public void messageBox(String message) {
        Log.i(TAG, "Message received: " + message);

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.MESSAGE, message));
    }

    @Keep
    public void callEndedCallback() {
        Log.i(TAG, "Call ended received!");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.ENDED));
    }

    @Keep
    public void callStartedCallback() {
        Log.i(TAG, "Call started received!");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.STARTED));
    }

    @Keep
    public void joiningConferenceCallback() {
        Log.i(TAG, "call joining received");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.CALLING));
    }

    @Keep
    public void loginSuccessfulCallback(String userEntityId) {
        Log.i(TAG, "Login Successful received!, ID: " + userEntityId);

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.LOGIN, userEntityId));
    }

    @Keep
    public void logoutSuccessfulCallback() {
        Log.i(TAG, "Logout Successful received!");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.LOGOUT));
    }

    @Keep
    public void libraryStartedCallback() {
        Log.i(TAG, "Library started received!");
        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.INIT_PASS, true));
    }

    @Keep
    public void onError() {
        Log.e(TAG, "Error received.");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.ERROR));
    }

    @Keep
    public void callbackActiveUsersInfo(String info) {
        Log.i(TAG, "Info received.");

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.MESSAGE, info));
    }

    @SuppressWarnings("JniMissingFunction")
    public native long Construct(String machineId, String caFileName, String logDir, String pathDir, String logLevel, Activity activity);

    @SuppressWarnings("JniMissingFunction")
    public native void Dispose();

    @SuppressWarnings("JniMissingFunction")
    public native void requestUsersEnergy();

    @SuppressWarnings("JniMissingFunction")
    public native void AutoStartMicrophone(boolean autoStart);

    @SuppressWarnings("JniMissingFunction")
    public native void AutoStartCamera(boolean autoStart);

    @SuppressWarnings("JniMissingFunction")
    public native void AutoStartSpeaker(boolean autoStart);

    @SuppressWarnings("JniMissingFunction")
    public native void Login(String portal, String username, String password);

    @SuppressWarnings("JniMissingFunction")
    public native void Render();

    @SuppressWarnings("JniMissingFunction")
    public native void RenderRelease();

    @SuppressWarnings("JniMissingFunction")
    public native void HideToolBar(boolean hide);

    @SuppressWarnings("JniMissingFunction")
    public native void SetCameraDevice(int camera);

    @SuppressWarnings("JniMissingFunction")
    public native void CycleCamera();

    /**
     * 0 - None
     * 1 - Pip
     * 2 - Dock
     *
     * @param mode described above
     */
    @SuppressWarnings("JniMissingFunction")
    public native void SetPreviewMode(int mode);

    @SuppressWarnings("JniMissingFunction")
    public native void ConfigAutoLogin(boolean enable);

    @SuppressWarnings("JniMissingFunction")
    public native void Resize(int width, int height);

    @SuppressWarnings("JniMissingFunction")
    public native int SendAudioFrame(byte[] frame, int numSamples,
                                     int sampleRate, int numChannels, int bitsPerSample);

    @SuppressWarnings("JniMissingFunction")
    public native int GetAudioFrame(byte[] frame, int numSamples,
                                    int sampleRate, int numChannels, int bitsPerSample);

    @SuppressWarnings("JniMissingFunction")
    public native int SendVideoFrame(byte[] frame, String fourcc, int width,
                                     int height, int orientation, boolean mirrored);

    @SuppressWarnings("JniMissingFunction")
    public native void TouchEvent(int id, int type, int x, int y);

    @SuppressWarnings("JniMissingFunction")
    public native void SetOrientation(int orientation);

    @SuppressWarnings("JniMissingFunction")
    public native void MuteCamera(boolean muteCamera);

    @SuppressWarnings("JniMissingFunction")
    public native void MuteMicrophone(boolean muteMic);

    /**
     * 0 for participant to see self video,
     * 1 for disabling self video
     * 2 for enabling self video only when there are no other participant in conference.
     *
     * @param policy described above
     */
    @SuppressWarnings("JniMissingFunction")
    public native void SetLoopbackPolicy(int policy);

    @SuppressWarnings("JniMissingFunction")
    public native void EnterBackground();

    @SuppressWarnings("JniMissingFunction")
    public native void EnterForeground();

    @SuppressWarnings("JniMissingFunction")
    public native void StartConferenceMedia();

    @SuppressWarnings("JniMissingFunction")
    public native void SetEchoCancellation(boolean aecenable);

    @SuppressWarnings("JniMissingFunction")
    public native void SetSpeakerVolume(int volume);

    @SuppressWarnings("JniMissingFunction")
    public native void DisableShareEvents();

    @SuppressWarnings("JniMissingFunction")
    public native int getNoOfparticipant();

    @SuppressWarnings("JniMissingFunction")
    public native void JoinRoomLink(String portal, String key, String userName, String pin);

    @SuppressWarnings("JniMissingFunction")
    public native void CancelJoin();

    @SuppressWarnings("JniMissingFunction")
    public native void LeaveConference();

    @SuppressWarnings("JniMissingFunction")
    public native String[] GetMicroPhoneList();

    @SuppressWarnings("JniMissingFunction")
    public native String[] GetSpeakerList();

    @SuppressWarnings("JniMissingFunction")
    public native void SetMicValue(int micNum);

    @SuppressWarnings("JniMissingFunction")
    public native void SetSpeakerValue(int spkNum);

    @SuppressWarnings("JniMissingFunction")
    public native void SetOnJoinConfiguration(boolean isCameraMuteOnJoin, boolean isSpeakerMuteOnJoin, boolean isMicMuteOnJoin);

    @SuppressWarnings("JniMissingFunction")
    public native void muteSpeaker();

    @SuppressWarnings("JniMissingFunction")
    public native void unMuteSpeaker();

    @SuppressWarnings("JniMissingFunction")
    public native void muteAudio();

    @SuppressWarnings("JniMissingFunction")
    public native void unMuteAudio();

    @SuppressWarnings("JniMissingFunction")
    public native void setStatsVisibility(boolean visibility);

    @SuppressWarnings("JniMissingFunction")
    public native void logSpeakerData();

    @SuppressWarnings("JniMissingFunction")
    public native void SetBackgroundColorToBlack();

    @SuppressWarnings("JniMissingFunction")
    public native void Logout();

    @SuppressWarnings("JniMissingFunction")
    public native void CancelLogin();

    @SuppressWarnings("JniMissingFunction")
    public native void PortalServiceJoinConference(String eid);

    @SuppressWarnings("JniMissingFunction")
    public native void requestActiveUsersInfo();

    @SuppressWarnings("JniMissingFunction")
    public native void Configure();

    @SuppressWarnings("JniMissingFunction")
    public native boolean IsLinkAvailable();

    @SuppressWarnings("JniMissingFunction")
    public native void SetPixelDensity(double density);

    @SuppressWarnings("JniMissingFunction")
    public native void SendGroupChatMessage(String message);

    @Keep
    public void onGroupChatMessageEvent(String displayName, String message) {
        Log.i(TAG, "group chat message received! Name: " + displayName + ", Message: " + message);

        EventBus.getDefault().post(new HomeBus<>(HomeBus.Call.GROUP_CHAT_MESSAGE, displayName, message));
    }
}