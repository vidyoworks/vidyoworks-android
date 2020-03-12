#include <jni.h>
#include <stdio.h>
#include <string.h>
#include "VidyoClient.h"
#include "include/AndroidDebug.h"
#include <pthread.h>
#include <unistd.h>

#include "VidyoClientPrivate.h"
#include "VidyoClientParameters.h"

VidyoClientRequestParticipants participant;
jobject applicationJniObj = 0;
JavaVM* global_vm = 0;
static VidyoBool joinStatus = 0;
int xCoordinate;
int yCoordinate;
static VidyoBool allVideoDisabled = 0;

void SampleSwitchCamera(const char *name);
void SampleStartConference();
void SampleEndConference();
void SampleLoginSuccessful(const char *name);
void SampleLogOutSuccessful();
void LibraryStarted();
void GetParticipants();
void SendErrorMessage();
void SendActiveUsersInfo(const char *info);

/* Declare method to get it referenced upper in the code */
void OnGroupChatMessageEvent(const char *name, const char *message);

void createDetailsEntityFromDetailsAtInfo(JNIEnv *env, jobject *deObject, VidyoClientRequestParticipantDetails* entityInfo);
void createDetailsEntity(JNIEnv *env, jobject *detailsObject, const char* name, const char* uri, const char* type);

//void ParticipantChangeCall(int noOfParticipants);

void logout();

void SetCoordinates(int x, int y)
{
	LOGI("***** **** ***** set coordinates  width=%d height=%d\n", x, y);
	xCoordinate = x;
	yCoordinate = y;
}

#define NUMBER_OF_CYCLES_WAIT_FOR_RESIZE_RESTORE		5
void * _TryResizeLater(void *val) {
	int i = 0;
	for(i = 0; i < NUMBER_OF_CYCLES_WAIT_FOR_RESIZE_RESTORE; ++i)
	{
		LOGI("***** **** ***** sleeping");
		sleep(1);
	}
	LOGI("***** **** ***** In another thread active Resize width=%d height=%d\n", xCoordinate, yCoordinate);
	doResize(xCoordinate,yCoordinate);
	return NULL;
}


void TryResizeLater()
{
	pthread_t t;

	LOGI("***** **** ***** creating thread");

	//Launch a thread
	pthread_create(&t, NULL, _TryResizeLater, NULL);
}

// Callback for out-events from VidyoClient
#define PRINT_EVENT(X) if(event==X) LOGI("GuiOnOutEvent recieved %s", #X);
void GuiOnOutEventCallback(VidyoClientOutEvent event, VidyoVoidPtr param, VidyoUint paramSize, VidyoVoidPtr data)
{
	LOGI("GuiOnOutEvent enter Event = %d\n",(int) event);
	if(event == VIDYO_CLIENT_OUT_EVENT_LICENSE)
	{
		VidyoClientOutEventLicense *eventLicense;
		eventLicense = (VidyoClientOutEventLicense *) param;

		VidyoUint error = eventLicense->error;
		VidyoUint vmConnectionPath = eventLicense->vmConnectionPath;
		VidyoBool OutOfLicenses = eventLicense->OutOfLicenses;

		LOGI("License Error: errorid=%d vmConnectionPath=%d OutOfLicense=%d\n", error, vmConnectionPath, OutOfLicenses);
	}
	else if(event == VIDYO_CLIENT_OUT_EVENT_SIGN_IN)
	{
		VidyoClientOutEventSignIn *eventSignIn;
		eventSignIn = (VidyoClientOutEventSignIn *) param;

		VidyoUint activeEid = eventSignIn->activeEid;
		VidyoBool signinSecured = eventSignIn->signinSecured;

		// SendActiveId(activeEid);

		LOGE("activeEid=%d signinSecured=%d\n", activeEid, signinSecured);

		/*
		 * If the EID is not setup, it will return activeEid = 0
		 * in this case, we invoke the license request using below event
		 */
		if(!activeEid)
			(void)VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_LICENSE, NULL, 0);
    }
    else if(event == VIDYO_CLIENT_OUT_EVENT_SIGNED_IN)
	{
        VidyoClientRequestCurrentUser user_id;
        VidyoUint ret = VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CURRENT_USER, &user_id, sizeof(user_id));

        LOGE("SG: logged in with %d. user_id.CurrentUserID: %s, user_id.CurrentUserDisplay: %s , Entity ID: %s",
            ret, user_id.currentUserID, user_id.currentUserDisplay, user_id.entityId);

		SampleLoginSuccessful(user_id.entityId);
    }
    else if(event == VIDYO_CLIENT_OUT_EVENT_SIGNED_OUT)
    {
             // Send message to Client/application
     		SampleLogOutSuccessful();
    }
     // Required to get Participant list during conference
    else if(event == VIDYO_CLIENT_OUT_EVENT_PARTICIPANTS_CHANGED)
    {
    	LOGE("Join Conference Event - received VIDYO_CLIENT_OUT_EVENT_PARTICIPANTS_CHANGED\n");

        VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_PARTICIPANTS, &participant,sizeof(VidyoClientRequestParticipants));
        LOGE("No of participant::%d", participant.numberParticipants);

        int i = 0;
        for(i = 0; i < participant.numberParticipants; ++i)
	    {
	        VidyoClientRequestParticipantDetailsAt request;
	        request.index = i;
            VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_PARTICIPANT_DETAILS_AT, &request, sizeof(VidyoClientRequestParticipantDetailsAt));

            VidyoClientRequestParticipantDetails detailsAt = request.details;

            LOGE("Participant data:: Index: %d, Name: %s, EntityId: %s, Type: %u", i, detailsAt.name, detailsAt.entityID,
            detailsAt.participantType);
	    }
    }
	else if(event == VIDYO_CLIENT_OUT_EVENT_CONFERENCE_ACTIVE)
	{
		LOGI("Join Conference Event - received VIDYO_CLIENT_OUT_EVENT_CONFERENCE_ACTIVE\n");
        SampleStartConference();
		joinStatus = 1;
		LOGI("***** **** ***** active Resize width=%d height=%d\n", xCoordinate, yCoordinate);
		doResize(xCoordinate,yCoordinate);
		TryResizeLater(NULL);
	}
	else if(event == VIDYO_CLIENT_OUT_EVENT_CONFERENCE_ENDED)
	{
		LOGI("Left Conference Event\n");
		SampleEndConference();
		joinStatus = 0;
	}
	else if(event == VIDYO_CLIENT_OUT_EVENT_INCOMING_CALL)
	{
		LOGW("VIDYO_CLIENT_OUT_EVENT_INCOMING_CALL\n");
		VidyoBool ret = VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_ANSWER, NULL, 0);
		LOGW("SG: VIDYO_CLIENT_OUT_EVENT_INCOMING_CALL %d.", ret);
	}
    else if(event == VIDYO_CLIENT_OUT_EVENT_ADD_SHARE)
           {
               VidyoClientRequestWindowShares shareRequest;
               VidyoUint result;

               LOGI("VIDYO_CLIENT_OUT_EVENT_ADD_SHARE\n");
               memset(&shareRequest, 0, sizeof(shareRequest));
               shareRequest.requestType = LIST_SHARING_WINDOWS;
                VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_WINDOW_SHARES, &shareRequest, sizeof(shareRequest));
               if (result != VIDYO_CLIENT_ERROR_OK)
               {
                   LOGE("VIDYO_CLIENT_REQUEST_GET_WINDOW_SHARES failed");
               }
               else
               {
                   LOGI("VIDYO_CLIENT_REQUEST_GET_WINDOW_SHARES success:%d, %d", shareRequest.shareList.numApp, shareRequest.shareList.currApp);

                   shareRequest.shareList.newApp = shareRequest.shareList.currApp = 1;
                   shareRequest.requestType = ADD_SHARING_WINDOW;

                   result = VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_WINDOW_SHARES,
                                                     &shareRequest,
                                                     sizeof(shareRequest));

                   if (result != VIDYO_CLIENT_ERROR_OK)
                   {
                       LOGE("VIDYO_CLIENT_REQUEST_SET_WINDOW_SHARES failed\n");

                   }
                   else
                   {
                       LOGI("VIDYO_CLIENT_REQUEST_SET_WINDOW_SHARES success\n");
                   }
               }
       	}
	else if (event == VIDYO_CLIENT_OUT_EVENT_DEVICE_SELECTION_CHANGED)
	{
		VidyoClientOutEventDeviceSelectionChanged *eventOutDeviceSelectionChg = (VidyoClientOutEventDeviceSelectionChanged *)param;

		if (eventOutDeviceSelectionChg->changeType == VIDYO_CLIENT_USER_MESSAGE_DEVICE_SELECTION_CHANGED)
		{
			if (eventOutDeviceSelectionChg->deviceType == VIDYO_CLIENT_DEVICE_TYPE_VIDEO)
			{
				SampleSwitchCamera((char *)eventOutDeviceSelectionChg->newDeviceName);
			}
		}
	}
	else if (event == VIDYO_CLIENT_OUT_EVENT_LOGIC_STARTED)
	{
		LOGI("Library Started Event\n");
		LibraryStarted();
	} else if (event == VIDYO_CLIENT_OUT_EVENT_ROOM_LINK)
	{
	    VidyoClientOutEventRoomLink *roomLinkEvent;
        roomLinkEvent = (VidyoClientOutEventRoomLink *) param;

        VidyoClientError error = roomLinkEvent->error;
        VidyoClientRoomLinkState state = roomLinkEvent->state;

        LOGI("Room link event: error=%u state=%u", error, state);

        if (error != VIDYO_CLIENT_ERROR_NONE) {
            LOGE("Room link error occurred!");
            SendErrorMessage();
        }
	} else if (event == VIDYO_CLIENT_OUT_EVENT_REMOTE_SOURCE_REMOVED)
    {
        LOGI("Received source event: VIDYO_CLIENT_OUT_EVENT_REMOTE_SOURCE_REMOVED");

        VidyoClientOutEventRemoteSourceChanged *sourceRemoved;
        sourceRemoved = (VidyoClientOutEventRemoteSourceChanged *) param;

        LOGI("Received source event: type=%u display=%s sourceName=%s",
            sourceRemoved->sourceType, sourceRemoved->displayName, sourceRemoved->sourceName);

    } else if (event == VIDYO_CLIENT_OUT_EVENT_REMOTE_SOURCE_ADDED)
    {
        LOGI("Received source event: VIDYO_CLIENT_OUT_EVENT_REMOTE_SOURCE_ADDED");
    } else if (event == VIDYO_CLIENT_OUT_EVENT_GROUP_CHAT) {
        LOGI("Received event: VIDYO_CLIENT_OUT_EVENT_GROUP_CHAT");

        VidyoClientOutEventGroupChat *event = (VidyoClientOutEventGroupChat*) param;

        /* Send over to the Java side */
        OnGroupChatMessageEvent(event->displayName, event->message);
     }
}

static JNIEnv *getJniEnv(jboolean *isAttached)
{
	int status;
	JNIEnv *env;
	*isAttached = 0;

	status = (*global_vm)->GetEnv(global_vm, (void **) &env, JNI_VERSION_1_4);
	if (status < 0) 
	{
		//LOGE("getJavaEnv: Failed to get Java VM");
		status = (*global_vm)->AttachCurrentThread(global_vm, &env, NULL);
		if(status < 0) 
		{
			LOGE("getJavaEnv: Failed to get Attach Java VM");
			return NULL;
		}
		//LOGE("getJavaEnv: Attaching to Java VM");
		*isAttached = 1;
	}

	return env;
}

static jmethodID getApplicationJniMethodId(JNIEnv *env, jobject obj, const char* methodName, const char* methodSignature)
{
	jmethodID mid;
	jclass appClass;

	if (obj == NULL)
	{
	   LOGE("getApplicationJniMethodId - getApplicationJniMethodId: Cannot callback. JNI ref object is null.");
	   return NULL;
	}

	appClass = (*env)->GetObjectClass(env, obj);
	if (!appClass) 
	{
		LOGE("getApplicationJniMethodId - getApplicationJniMethodId: Failed to get applicationJni obj class");
		return NULL;
	}
	
	mid = (*env)->GetMethodID(env, appClass, methodName, methodSignature);
	if (mid == NULL)
	{
		LOGE("getApplicationJniMethodId - getApplicationJniMethodId: Failed to get %s method", methodName);
		return NULL;
	}
	
	return mid;
}

void LibraryStarted()
{
	jboolean isAttached;
	JNIEnv *env;
	jmethodID mid;
	jstring js;
	LOGE("LibraryStarted Begin");
	env = getJniEnv(&isAttached);
	if (env == NULL)
		goto FAIL0;

	mid = getApplicationJniMethodId(env, applicationJniObj, "libraryStartedCallback", "()V");

	(*env)->CallVoidMethod(env, applicationJniObj, mid);

	if (isAttached)
	{
		(*global_vm)->DetachCurrentThread(global_vm);
	}
	LOGE("LibraryStarted End");
	return;
	FAIL1:
	if (isAttached)
	{
		(*global_vm)->DetachCurrentThread(global_vm);
	}
	FAIL0:
	LOGE("LibraryStarted FAILED");
	return;
}

void GetParticipants(JNIEnv *env, jobject thisObj)

{
	int i;
	VidyoClientRequestParticipants participant;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_PARTICIPANTS, &participant,
	sizeof(VidyoClientRequestParticipants));
	LOGI("No of participant::%d",participant.numberParticipants);

}

void SampleStartConference()
{
    jboolean isAttached;
    JNIEnv *env;
    jmethodID mid;
    jstring js;
    LOGE("SampleStartConference Begin");
    env = getJniEnv(&isAttached);
    if (env == NULL)
        goto FAIL0;
    
    mid = getApplicationJniMethodId(env, applicationJniObj, "callStartedCallback", "()V");
    if (mid == NULL)
        goto FAIL1;
    
    (*env)->CallVoidMethod(env, applicationJniObj, mid);
	
    if (isAttached)
    {
        (*global_vm)->DetachCurrentThread(global_vm);
    }
    LOGE("SampleStartConference End");
    return;
FAIL1:
    if (isAttached)
    {
        (*global_vm)->DetachCurrentThread(global_vm);
    }
FAIL0:
    LOGE("SampleStartConference FAILED");
    return;
}

void SampleLoginSuccessful(const char *eId)
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID mid;
        jstring js;
        LOGE("SampleLoginSuccessful Begin");
        env = getJniEnv(&isAttached);
        if (env == NULL)
                goto FAIL0;

        mid = getApplicationJniMethodId(env, applicationJniObj, "loginSuccessfulCallback", "(Ljava/lang/String;)V");
        if (mid == NULL)
                goto FAIL1;

        js = (*env)->NewStringUTF(env, eId);
        (*env)->CallVoidMethod(env, applicationJniObj, mid, js);

		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
        LOGE("SampleLoginSuccessful End");
        return;
FAIL1:
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("SampleLoginSuccessful FAILED");
        return;
}

void SampleLogOutSuccessful()
{
    jboolean isAttached;
    JNIEnv *env;
    jmethodID mid;
    jstring js;
    LOGE("SampleLogoutSuccessful Begin");
    env = getJniEnv(&isAttached);
    if (env == NULL)
        goto FAIL0;

    mid = getApplicationJniMethodId(env, applicationJniObj, "logoutSuccessfulCallback", "()V");
    if (mid == NULL)
        goto FAIL1;

    (*env)->CallVoidMethod(env, applicationJniObj, mid);

    if (isAttached)
    {
        (*global_vm)->DetachCurrentThread(global_vm);
    }
    LOGE("SampleLogoutSuccessful End");
    return;
FAIL1:
    if (isAttached)
    {
        (*global_vm)->DetachCurrentThread(global_vm);
    }
FAIL0:
    LOGE("SampleLogoutSuccessful FAILED");
    return;
}

void SampleEndConference()
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID mid;
        jstring js;
        LOGE("SampleEndConference Begin");
        env = getJniEnv(&isAttached);
        if (env == NULL)
                goto FAIL0;

        mid = getApplicationJniMethodId(env, applicationJniObj, "callEndedCallback", "()V");
        if (mid == NULL)
                goto FAIL1;

        (*env)->CallVoidMethod(env, applicationJniObj, mid);
	
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
        LOGE("SampleEndConference End");
        return;
FAIL1:
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("SampleEndConference FAILED");
        return;
}

void SampleSwitchCamera(const char *name)
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID mid;
        jstring js;
        LOGE("SampleSwitchCamera Begin");
        env = getJniEnv(&isAttached);
        if (env == NULL)
                goto FAIL0;

        mid = getApplicationJniMethodId(env, applicationJniObj, "cameraSwitchCallback", "(Ljava/lang/String;)V");
        if (mid == NULL)
                goto FAIL1;

        js = (*env)->NewStringUTF(env, name);
        (*env)->CallVoidMethod(env, applicationJniObj, mid, js);
	
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
        LOGE("SampleSwitchCamera End");
        return;
FAIL1:
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("SampleSwitchCamera FAILED");
        return;
}

void SendErrorMessage()
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID mid;
        jstring js;
        LOGE("Send error Begin");
        env = getJniEnv(&isAttached);
        if (env == NULL)
                goto FAIL0;

        mid = getApplicationJniMethodId(env, applicationJniObj, "onError", "()V");
        if (mid == NULL)
                goto FAIL1;

        (*env)->CallVoidMethod(env, applicationJniObj, mid);

		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
        LOGE("Send error End");
        return;
FAIL1:
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("Send error FAILED");
        return;
}

void SendActiveUsersInfo(const char *info)
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID mid;
        jstring js;
        LOGE("SendActiveUsersInfo Begin");
        env = getJniEnv(&isAttached);
        if (env == NULL)
                goto FAIL0;

        mid = getApplicationJniMethodId(env, applicationJniObj, "callbackActiveUsersInfo", "(Ljava/lang/String;)V");
        if (mid == NULL)
                goto FAIL1;

        js = (*env)->NewStringUTF(env, info);
        (*env)->CallVoidMethod(env, applicationJniObj, mid, js);

		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
        LOGE("SendActiveUsersInfo End");
        return;
FAIL1:
		if (isAttached)
		{
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("SendActiveUsersInfo FAILED");
        return;
}

static jobject * SampleInitCacheClassReference(JNIEnv *env, const char *classPath) 
{

	LOGE("cacheClassReference: Start handler resolution.");

	jclass cls = (*env)->FindClass(env, classPath);
	if (!cls)
	{
		LOGE("cacheClassReference: Failed to find class %s", classPath);
		return ((jobject*)0);
	}

	jmethodID methodID = (*env)->GetMethodID(env, cls, "<init>", "()V");
	if (!methodID)
	{
		LOGE("cacheClassReference: Failed to construct %s", classPath);
		return ((jobject*)0);
	}

	jobject handlerObj = (*env)->NewObject(env, cls, methodID);
	if (!handlerObj)
	{
		LOGE("cacheClassReference: Failed to create object %s", classPath);
		return ((jobject*)0);
	}

	return (*env)->NewGlobalRef(env, handlerObj);
}

void doSetLogLevelsAndCategories(char* newLogging)
{
    if (newLogging != NULL)
    {
        if (strlen(newLogging) > 199)
        {
            LOGE("New logging string too long!");
        }
        else
        {
           
            LOGE("Log String set to %s\n", newLogging);
            
            
            VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_LOG_LEVELS_AND_CATEGORIES, newLogging, sizeof(newLogging));
        }
    }
  
}

void disablePinnedUI() {
    VidyoClientFeatureControl featureControl = {0};
    VidyoClientFeatureControlConstructDefault(&featureControl);
    featureControl.disableUiPinning = VIDYO_TRUE;

    if (VidyoClientSetOptionalFeatures(&featureControl) == VIDYO_FALSE) {
        LOGE("Failed to change UI pinning state");
    }
}

JNIEXPORT jint Java_com_vidyo_works_support_JniBridge_getNoOfparticipant(JNIEnv *env, jobject jobj) {
	return (jint)participant.numberParticipants;

}

JNIEXPORT void Java_com_vidyo_works_support_JniBridge_Construct(JNIEnv* env, jobject javaThis,
                   jstring machineId, jstring caFilename, jstring logDir, jstring pathDir, jstring logLevel,
                   jobject defaultActivity) {

	FUNCTION_ENTRY

	VidyoClientAndroidRegisterDefaultVM(global_vm);
	VidyoClientAndroidRegisterDefaultApp(env, defaultActivity);

	const char *machineIdC = (*env)->GetStringUTFChars(env, machineId, NULL);
	LOGE("ApplicationJni_Construct:machineIdC=%s\n", machineIdC);

	const char *pathDirC = (*env)->GetStringUTFChars(env, pathDir, NULL);
	LOGE("ApplicationJni_Construct:pathDirC=%s\n", pathDirC);

	const char *logDirC = (*env)->GetStringUTFChars(env, logDir, NULL);
	LOGE("ApplicationJni_Construct:LogDir=%s\n", logDirC);

	const char *certificatesFileNameC = (*env)->GetStringUTFChars(env, caFilename, NULL);
	LOGE("ApplicationJni_Construct:certificatesFileNameC=%s\n", certificatesFileNameC);

	const char *logLevelC = (*env)->GetStringUTFChars(env, logLevel, NULL);
	LOGE("ApplicationJni_Construct:logLevel=%s\n", logLevelC);

	VidyoRect videoRect = { (VidyoInt) (0), (VidyoInt) (0), (VidyoUint) (100), (VidyoUint) (100) };

	applicationJniObj = SampleInitCacheClassReference(env, "com/vidyo/works/support/JniBridge");

	VidyoClientConsoleLogConfigure(VIDYO_CLIENT_CONSOLE_LOG_CONFIGURATION_ALL);

	VidyoClientLogParams logParam = {0};

    logParam.logLevelsAndCategories = logLevelC;
    logParam.logSize = 1000000;
    logParam.logBaseFileName = "VidyoLog_";
    logParam.pathToLogDir = logDirC;
    logParam.pathToDumpDir = logDirC;
    logParam.pathToConfigDir = pathDirC;

	LOGE("ApplicationJni_Construct: logelevel=%s, certifcateFileName=%s, configDir=%s, logDir=%s!\n", logParam.logLevelsAndCategories,
			certificatesFileNameC, pathDirC, logDirC);

	if (VidyoClientInitialize(GuiOnOutEventCallback, NULL, &logParam) == VIDYO_FALSE) {
	    LOGE("INITIALIZE: Failed");
	} else {
        LOGE("INITIALIZE: Success");
	}

    /* Uncomment to disable UI pin */
	// disablePinnedUI();

    AndroidClientSetMachineID(machineIdC);

	VidyoBool returnValue = VidyoClientStart(GuiOnOutEventCallback, NULL,
			&logParam, (VidyoWindowId) (0), &videoRect, NULL, NULL, NULL );
	if (returnValue) {
		LOGI("VidyoClientStart() was a SUCCESS\n");
	} else {
		//start failed
		LOGE("ApplicationJni_Construct VidyoClientStart() returned error!\n");
	}

	// AppCertificateStoreInitialize(logDirC,certificatesFileNameC,NULL);

	FUNCTION_EXIT;
}

JNIEXPORT void Java_com_vidyo_works_support_JniBridge_Login(JNIEnv* env, jobject javaThis, jstring vidyoportalName, jstring userName, jstring passwordName) {

	FUNCTION_ENTRY;
	LOGI("Java_com_vidyo_works_support_JniBridge_Login() enter\n");

	const char *portalC = (*env)->GetStringUTFChars(env, vidyoportalName, NULL);
	const char *usernameC = (*env)->GetStringUTFChars(env, userName, NULL);
	const char *passwordC = (*env)->GetStringUTFChars(env, passwordName, NULL);

	LOGI("Starting Login Process\n");
	VidyoClientInEventLogIn event = {0};

	strlcpy(event.portalUri, portalC, sizeof(event.portalUri));
	strlcpy(event.userName, usernameC, sizeof(event.userName));
	strlcpy(event.userPass, passwordC, sizeof(event.userPass));

	LOGI("logging in with portalUri %s user %s ", event.portalUri, event.userName);
    
	VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_LOGIN, &event, sizeof(VidyoClientInEventLogIn));
 	FUNCTION_EXIT;
}


JNIEXPORT void Java_com_vidyo_works_support_JniBridge_JoinRoomLink(JNIEnv* env, jobject javaThis,
                                                                       jstring vidyoportalName, jstring key, jstring displayName, jstring pin) {
    
    FUNCTION_ENTRY;
    LOGI("Java_com_vidyo_works_support_JniBridge_JoinRoomLink() enter\n");
    
    const char *portalC = (*env)->GetStringUTFChars(env, vidyoportalName, NULL);
    const char *keyC = (*env)->GetStringUTFChars(env, key, NULL);
    const char *displayNameC = (*env)->GetStringUTFChars(env, displayName, NULL);
    const char *pinC = (*env)->GetStringUTFChars(env, pin, NULL);
    
    LOGI("Starting JoinRoomLink Process\n");
    
    VidyoClientInEventRoomLink event = {0};
    
    strlcpy(event.portalUri, portalC, sizeof(event.portalUri));
    strlcpy(event.roomKey, keyC, sizeof(event.roomKey));
    strlcpy(event.displayName, displayNameC, sizeof(event.displayName));
    strlcpy(event.pin, pinC, sizeof(event.pin));
    
    LOGI("joiningRoomlink with portalUri %s key %s ", event.portalUri, event.roomKey);
    
    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_ROOM_LINK, &event, sizeof(VidyoClientInEventRoomLink));
    FUNCTION_EXIT;
}

JNIEXPORT void Java_com_vidyo_works_support_JniBridge_CancelJoin(JNIEnv* env, jobject javaThis) {

    FUNCTION_ENTRY;
    LOGI("Java_com_vidyo_works_support_JniBridge_CancelJoin() enter\n");
    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_CANCEL, NULL, 0);
    FUNCTION_EXIT;
}

JNIEXPORT void Java_com_vidyo_works_support_JniBridge_LeaveConference(JNIEnv* env, jobject javaThis) {

    FUNCTION_ENTRY;
    LOGI("Java_com_vidyo_works_support_JniBridge_LeaveConference() enter\n");

    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_LEAVE, NULL, 0);

    FUNCTION_EXIT;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_Dispose(JNIEnv *env, jobject jObj2)
{
	FUNCTION_ENTRY;
	if (VidyoClientStop())
		LOGI("VidyoClientStop() SUCCESS!!\n");
        
	else
		LOGE("VidyoClientStop() FAILURE!!\n");

	FUNCTION_EXIT;
}


JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM *vm, void *pvt )
{
	FUNCTION_ENTRY;
	LOGI("JNI_OnLoad called\n");
	global_vm = vm;
	FUNCTION_EXIT;
	return JNI_VERSION_1_4;
}

JNIEXPORT void JNICALL JNI_OnUnload( JavaVM *vm, void *pvt )
{
	FUNCTION_ENTRY
	LOGE("JNI_OnUnload called\n");
	FUNCTION_EXIT
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_Render(JNIEnv *env, jobject jObj2)
{
//	FUNCTION_ENTRY;
	doRender();
//	FUNCTION_EXIT;
}


JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_RenderRelease(JNIEnv *env, jobject jObj2)
{
	FUNCTION_ENTRY;
	doSceneReset();
	FUNCTION_EXIT;
}

 void JNICALL Java_com_vidyo_works_support_JniBridge_Resize(JNIEnv *env, jobject jobj, jint width, jint height)
{
	FUNCTION_ENTRY;
	LOGI("***** **** ***** JNI Resize width=%d height=%d\n", width, height);
	SetCoordinates(width, height);
	doResize( (VidyoUint)width, (VidyoUint)height);
	FUNCTION_EXIT;
}


JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_TouchEvent(JNIEnv *env, jobject jobj, jint id, jint type, jint x, jint y)
{
	FUNCTION_ENTRY;
	doTouchEvent((VidyoInt)id, (VidyoInt)type, (VidyoInt)x, (VidyoInt)y);
	FUNCTION_EXIT;
}


JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetOrientation(JNIEnv *env, jobject jobj,  jint orientation)
{
FUNCTION_ENTRY;

        VidyoClientOrientation newOrientation = VIDYO_CLIENT_ORIENTATION_UP;

        //translate LMI orienation to client orientation
        switch(orientation) {
                case 0: newOrientation = VIDYO_CLIENT_ORIENTATION_UP;
                                LOGI("VIDYO_CLIENT_ORIENTATION_UP");
                                break;
                case 1: newOrientation = VIDYO_CLIENT_ORIENTATION_DOWN;
                        LOGI("VIDYO_CLIENT_ORIENTATION_DOWN");
                        break;
                case 2: newOrientation = VIDYO_CLIENT_ORIENTATION_LEFT;
                        LOGI("VIDYO_CLIENT_ORIENTATION_LEFT");
                        break;
                case 3: newOrientation = VIDYO_CLIENT_ORIENTATION_RIGHT;
                        LOGI("VIDYO_CLIENT_ORIENTATION_RIGHT");
                        break;
        }

        doClientSetOrientation(newOrientation);

FUNCTION_EXIT;
return;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetCameraDevice(JNIEnv *env, jobject jobj, jint camera)
{
	VidyoClientRequestConfiguration requestConfig;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));

	requestConfig.currentCamera = camera;

	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_CycleCamera(JNIEnv *env, jobject jobj)
{
	VidyoClientRequestConfiguration requestConfig;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));

	int camera = requestConfig.currentCamera;

    int i;
	for (i = 0; i < requestConfig.numberCameras; ++i) {
        LOGE("Camera device:: Index: %d, Name: %s", i, requestConfig.cameras[i]);
    }

    if (camera == (requestConfig.numberCameras - 1)) {
        camera = 0;
    } else {
        camera++;
    }

    requestConfig.currentCamera = camera;

	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_Configure(JNIEnv *env, jobject jobj)
{
	VidyoClientRequestConfiguration requestConfig;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));

    //
    requestConfig.enableHideCameraOnJoin = 1;
    //

	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_ConfigAutoLogin(JNIEnv *env, jobject jobj, jboolean enable)
{
	FUNCTION_ENTRY

	VidyoClientRequestConfiguration requestConfig;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));
	requestConfig.enableAutoLogIn = enable;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));

	FUNCTION_EXIT
}


JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetPreviewMode(JNIEnv *env, jobject jobj, jint value)
{
	VidyoClientInEventPreview event;

	if (value == 2) {
		event.previewMode = VIDYO_CLIENT_PREVIEW_MODE_DOCK;
	} else if (value == 1) {
		event.previewMode = VIDYO_CLIENT_PREVIEW_MODE_PIP;
	} else { // 0
		event.previewMode = VIDYO_CLIENT_PREVIEW_MODE_NONE;
	}

	VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_PREVIEW, &event, sizeof(VidyoClientInEventPreview));
}

void _init()
{
	FUNCTION_ENTRY;
	LOGE("_init called\n");
	FUNCTION_EXIT;
}

void _fini()
{
	FUNCTION_ENTRY;
	LOGE("_fini called\n");
	FUNCTION_EXIT;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_EnterBackground(JNIEnv *env, jobject jobj)
{
    if (!allVideoDisabled) {
        // this would have the effect of stopping all video streams but self preview
        VidyoClientRequestSetBackground reqBackground = {0};
        reqBackground.willBackground = VIDYO_TRUE;
        (void) VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_BACKGROUND, &reqBackground, sizeof(reqBackground));

        allVideoDisabled = VIDYO_TRUE;
    }
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_EnterForeground(JNIEnv *env, jobject jobj)
{
	if (allVideoDisabled) {
        VidyoClientRequestSetBackground reqBackground = {0};
    	reqBackground.willBackground = VIDYO_FALSE;
        (void) VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_BACKGROUND, &reqBackground, sizeof(reqBackground));

    	// this would have the effect of enabling all video streams
    	allVideoDisabled = VIDYO_FALSE;
        // rearrangeSceneLayout();
    }
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_MuteCamera(JNIEnv *env, jobject jobj, jboolean MuteCamera)
{
	VidyoClientInEventMute event;
	event.willMute = MuteCamera;
	VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_MUTE_VIDEO, &event, sizeof(VidyoClientInEventMute));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_StartConferenceMedia(JNIEnv *env, jobject jobj)
{
    doStartConferenceMedia();
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_HideToolBar(JNIEnv* env, jobject jobj, jboolean disablebar)
{
	LOGI("Java_com_vidyo_works_support_JniBridge_HideToolBar() enter\n");
    VidyoClientInEventEnable event;
    event.willEnable = VIDYO_TRUE;
    VidyoBool ret = VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_ENABLE_BUTTON_BAR, &event,sizeof(VidyoClientInEventEnable));
    if (!ret)
        LOGW("Java_com_vidyo_works_support_JniBridge_HideToolBar() failed!\n");
}

// this function will enable echo cancellation
JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetEchoCancellation(JNIEnv *env, jobject jobj, jboolean aecenable)
{
	// get persistent configuration values
	  VidyoClientRequestConfiguration requestConfiguration;

	  VidyoUint ret = VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfiguration,
	                                                                             sizeof(requestConfiguration));
	  if (ret != VIDYO_CLIENT_ERROR_OK) {
	          LOGE("VIDYO_CLIENT_REQUEST_GET_CONFIGURATION returned error!");
	          return;
	  }

	  // modify persistent configuration values, based on current values of on-screen controls
	  if (aecenable) {
	          requestConfiguration.enableEchoCancellation = 1;
	  } else {
	          requestConfiguration.enableEchoCancellation = 0;
	  }

	  // set persistent configuration values
	  ret = VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_CONFIGURATION, &requestConfiguration,
	                                                           sizeof(requestConfiguration));
	  if (ret != VIDYO_CLIENT_ERROR_OK) {
	          LOGE("VIDYO_CLIENT_REQUEST_SET_CONFIGURATION returned error!");
	  }
	}
JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetSpeakerVolume(JNIEnv *env, jobject jobj, jint volume)
{
	//FUNCTION ENTRY
	VidyoClientRequestVolume volumeRequest;
	volumeRequest.volume = volume;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_SET_VOLUME_AUDIO_OUT, &volumeRequest,
		                                                           sizeof(volumeRequest));
	//FUNCTION EXIT
	return;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_DisableShareEvents(JNIEnv *env, jobject javaThisj)
{
	FUNCTION_ENTRY
	VidyoClientSendEvent (VIDYO_CLIENT_IN_EVENT_DISABLE_SHARE_EVENTS, 0, 0);
	LOGI("Disable Shares Called - Vimal");
	FUNCTION_EXIT;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_setStatsVisibility(JNIEnv *env, jobject jobj, jboolean visibility)
{
	FUNCTION_ENTRY
    doSetStatsVisibility(visibility);
	LOGI("Disable Shares Called - Vimal");
	FUNCTION_EXIT;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_logSpeakerData(JNIEnv *env, jobject jobj)
{
	FUNCTION_ENTRY
    VidyoClientRequestConfiguration requestConfig;
	VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CONFIGURATION, &requestConfig, sizeof(VidyoClientRequestConfiguration));

    LOGI("Number of speakers: %u", requestConfig.numberSpeakers);
    LOGI("Current speaker: %u", requestConfig.currentSpeaker);
	FUNCTION_EXIT;
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetBackgroundColorToBlack(JNIEnv *env, jobject jobj)
{
    VidyoClientInEventColor req;
    req.color.red = 0;
    req.color.green = 0;
    req.color.blue = 0;

    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_SET_BACKGROUND_COLOR, &req, sizeof(req));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_PortalServiceJoinConference(JNIEnv *env, jobject jobj, jstring eid) {
    //FUNCTION_ENTRY
    LOGI("calling portal service join conference");
    VidyoClientPortalServiceJoinConferenceRequest requestConfig;

    requestConfig.typeRequest = VIDYO_CLIENT_SERVICE_JOIN_CONFERENCE;

    const char *eidC = (*env)->GetStringUTFChars(env, eid, NULL);

    strlcpy(requestConfig.entityID, eidC, sizeof(requestConfig.entityID));
    // set to blank otherwise junk appears
    strlcpy(requestConfig.roomPIN, "", sizeof(requestConfig.roomPIN));
    // set to blank otherwise junk appears
    strlcpy(requestConfig.referenceNumber, "", sizeof(requestConfig.referenceNumber));
    // set to blank otherwise junk appears
    strlcpy(requestConfig.extension, "", sizeof(requestConfig.extension));

    requestConfig.muteCamera = VIDYO_FALSE;
    requestConfig.muteMicrophone = VIDYO_FALSE;
    requestConfig.muteSpeaker = VIDYO_FALSE;

    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_PORTAL_SERVICE, &requestConfig, sizeof(VidyoClientPortalServiceJoinConferenceRequest));
    //FUNCTION_EXIT
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_Logout(JNIEnv *env, jobject jObj2) {
    FUNCTION_ENTRY

    VidyoClientRequestCallState getCallState = {};

    // blocking request
    if (VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_CALL_STATE, &getCallState, sizeof(getCallState)) == VIDYO_CLIENT_ERROR_OK) {
        if (getCallState.callState != VIDYO_CLIENT_CALL_STATE_IDLE) {
            // cancel pending conference...
            LOGW("logout - call state not idle - canceling\n");
            VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_CANCEL, NULL, 0);
        }
        else {
            LOGI("logout - call state idle, logging out\n");
            logout();
        }
    }
    else {
        LOGW("could not get call state, logging out anyway\n");
        logout();
    }

    FUNCTION_EXIT
}

void logout() {
    FUNCTION_ENTRY
    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_SIGNOFF, 0, 0);
    LOGI("Logout\n");
    FUNCTION_EXIT
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_CancelLogin(JNIEnv *env, jobject jobj) {
    FUNCTION_ENTRY

    VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_LOGIN_CANCEL, NULL, 0);

    FUNCTION_EXIT
}

/* VIDYO_CLIENT_REQUEST_GET_AUDIO_ACTIVE_USERS API Impl */
JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_requestActiveUsersInfo(JNIEnv *env, jobject jobj) {
    FUNCTION_ENTRY

    VidyoClientRequestAudioActiveUsers requestData;
    VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_AUDIO_ACTIVE_USERS, &requestData, sizeof(VidyoClientRequestAudioActiveUsers));
    int size = sizeof(requestData.activeUsers) / sizeof(VidyoClientAudioActiveUser);

    LOGE("Get users size: %u", size);

    int iterator = 0;
	for(iterator = 0; iterator < size; ++iterator)
	{
	    VidyoClientAudioActiveUser activeUser = requestData.activeUsers[iterator];

        LOGE("Active users info (%u): URI: %s ENERGY: %u", iterator, activeUser.participantURI, activeUser.energy);
	}

    // SendActiveUsersInfo("...");
    FUNCTION_EXIT
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_MuteMicrophone(JNIEnv *env, jobject jobj, jboolean muteMic)
{
	VidyoClientInEventMute event;
	event.willMute = muteMic;
	VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_MUTE_AUDIO_IN, &event, sizeof(VidyoClientInEventMute));
}


JNIEXPORT jboolean JNICALL Java_com_vidyo_works_support_JniBridge_IsLinkAvailable(JNIEnv *env, jobject jobj)
{
    VidyoClientRequestBandwidthInfo reqBandwidthInfo = {};
    VidyoUint result = VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_GET_BANDWIDTH_INFO, &reqBandwidthInfo, sizeof(reqBandwidthInfo));

    if (result != VIDYO_CLIENT_ERROR_OK) {
        LOGE("Failed to get bandwidth info");
        return VIDYO_FALSE;
    }

    LOGI("Bandwidth actual receive info max: %d", reqBandwidthInfo.ActualRecvBwMax);

    // Limit to 10 kilobits per second
    if (reqBandwidthInfo.ActualRecvBwMax < 10) {
        return VIDYO_FALSE;
    }

    return VIDYO_TRUE;
}

void SendPrivateRequest(VidyoClientPrivateRequest request, void *param1, size_t param1Size, size_t param2){
    VidyoClientRequestPrivate requestPrivate = {0};
    requestPrivate.requestType = (VidyoUint)request;
    requestPrivate.requestData = param1;
    requestPrivate.requestDataSize = param1Size;
    requestPrivate.reserved = param2;

    VidyoClientSendRequest(VIDYO_CLIENT_REQUEST_PRIVATE, &requestPrivate, sizeof(VidyoClientRequestPrivate));
}

JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SetPixelDensity(JNIEnv *env, jobject javaThisj, jdouble density)
{
    FUNCTION_ENTRY
    VidyoClientPrivateRequestSetPixelDensity req  = {0};
    req.density = density;
    SendPrivateRequest(VIDYO_CLIENT_PRIVATE_REQUEST_SET_PIXEL_DENSITY, &req, sizeof(VidyoClientPrivateRequestSetPixelDensity), 0);

    FUNCTION_EXIT;
}

/* */
JNIEXPORT void JNICALL Java_com_vidyo_works_support_JniBridge_SendGroupChatMessage(JNIEnv *env, jobject jobj, jstring message)
{
	VidyoClientInEventGroupChat event;

    const char *messageC = (*env)->GetStringUTFChars(env, message, NULL);
	strlcpy(event.message, messageC, sizeof(event.message));

	VidyoBool result = VidyoClientSendEvent(VIDYO_CLIENT_IN_EVENT_GROUP_CHAT, &event, sizeof(VidyoClientInEventGroupChat));
	LOGI("Sending group chat message over. Result: %d", result);
}

/** Send group chat message to the Java layer through JNI */
void OnGroupChatMessageEvent(const char *name, const char *message)
{
        jboolean isAttached;
        JNIEnv *env;
        jmethodID methodID;

        jstring jName;
        jstring jMessage;

        LOGE("Send OnGroupChatEvent Begin");

        env = getJniEnv(&isAttached);
        if (env == NULL) goto FAIL0;

        methodID = getApplicationJniMethodId(env, applicationJniObj, "onGroupChatMessageEvent", "(Ljava/lang/String;Ljava/lang/String;)V");
        if (methodID == NULL) goto FAIL1;

        jName = (*env)->NewStringUTF(env, name);
        jMessage = (*env)->NewStringUTF(env, message);

        (*env)->CallVoidMethod(env, applicationJniObj, methodID, jName, jMessage);

		if (isAttached) {
			(*global_vm)->DetachCurrentThread(global_vm);
		}

        LOGE("Send OnGroupChatEvent End");
        return;
FAIL1:
		if (isAttached) {
			(*global_vm)->DetachCurrentThread(global_vm);
		}
FAIL0:
        LOGE("Send OnGroupChatEvent FAILED");
        return;
}