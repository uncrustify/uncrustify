// C/C++/Obj-C/Obj-C++ wrapper that exports C functions
//
// As an example, distributed with .c suffix,
// but depending on extension needs, can be
// compiled as C, C++, Obj-C or Obj-C++

#include <stdio.h>
#include <unistd.h>
#include "TestClassNativeHelper.h"

#undef NDEBUG

#if defined(__ANDROID__)

#define TCH_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TestClassNativeHelper", __VA_ARGS__)
#define TCH_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "TestClassNativeHelper", __VA_ARGS__)

#ifndef NDEBUG
#define TCH_LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "TestClassNativeHelper", __VA_ARGS__)
#else
#define TCH_LOGV(...)
#endif

#define TCH_ELOG TCH_LOGV("%s", __FUNCTION__)

void TCH_onTestClassUploadDidProgress(C_JNIEnv *env, jobject thiz, jint videoId, jdouble progress) {
    TCH_ELOG;
}

#ifndef TCH_NELEM
#define TCH_NELEM(x) ((int)(sizeof(x) / sizeof((x)[0])))
#endif

static const char *kTCHTestClassClass = "com/testclass/TestClass/TestClass";
static const char *kTCHTestClassLoader = "com.testclass.TestClass.TestClass";

static const char *kTCHTestClassNativeListenerClass = "com/testclass/TestClass/communication/TestClassNativeListener";
static const char *kTCHTestClassNativeListenerLoader = "com.testclass.TestClass.communication.TestClassNativeListener";

static JavaVM *TCH_vm = NULL;

static JNINativeMethod TCH_listenerMethods[] = {
    { "onTestClassUploadDidProgress", "(ID)V", (void *)TCH_onTestClassUploadDidProgress },
};

static C_JNIEnv *TCH_getEnv() {
    C_JNIEnv *ret = NULL;

    if (TCH_vm == NULL) {
        TCH_LOGE("TCH_getEnv failed, no JVM");
        return NULL;
    }

#if defined(__cplusplus)
    JNIEnv *env = NULL;

    if (TCH_vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        args.name = NULL;
        args.group = NULL;

        int attachStatus;
        if ((attachStatus = TCH_vm->AttachCurrentThread(&env, &args)) < 0) {
            TCH_LOGE("TCH_getEnv failed");
        }
    }
    ret = (C_JNIEnv *)env;
#else
    JNIEnv *env = NULL;

    if ((*TCH_vm)->GetEnv(TCH_vm, (void **)&env, JNI_VERSION_1_6) != JNI_OK) {
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        args.name = NULL;
        args.group = NULL;

        int attachStatus;
        if ((attachStatus = (*TCH_vm)->AttachCurrentThread(TCH_vm, &env, &args)) < 0) {
            TCH_LOGE("TCH_getEnv failed");
        }
    }
    ret = env;
#endif /* if defined(__cplusplus) */
    return ret;
}

static jclass TCH_loadClass(C_JNIEnv *env, jobject activity, const char *className) {
    jclass cls_Activity = (*env)->GetObjectClass((JNIEnv *)env, activity);
    jmethodID mid_getClassLoader = (*env)->GetMethodID((JNIEnv *)env, cls_Activity, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject obj_classLoader = (*env)->CallObjectMethod((JNIEnv *)env, activity, mid_getClassLoader);

    jclass cls_classLoader = (*env)->GetObjectClass((JNIEnv *)env, obj_classLoader);
    jmethodID mid_loadClass = (*env)->GetMethodID((JNIEnv *)env, cls_classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

    jstring _className = (*env)->NewStringUTF((JNIEnv *)env, className);
    jclass cls = (jclass)(*env)->CallObjectMethod((JNIEnv *)env, obj_classLoader, mid_loadClass, _className);

    (*env)->DeleteLocalRef((JNIEnv *)env, _className);

    if (!cls) {
        TCH_LOGE("Couldn't find class %s", className);
    }

    return cls;
}

#define TCH_str2(x)             # x
#define TCH_str(x)              TCH_str2(x)

#define TCH_CLASS(_class)       tchClass_ ## _class

#define TCH_LOCAL_CLASS(_class) local_tchClass_ ## _class

#define TCH_REGISTER_CLASS(_class) \
    jclass TCH_CLASS(_class) = NULL;

#define TCH_EXTERN_CLASS(_class) \
    jclass TCH_CLASS(_class);

#define TCH_METHOD(_method)     tchMethod_ ## _method

#define TCH_REGISTER_METHOD(_method) \
    jmethodID TCH_METHOD(_method) = NULL;

#define TCH_EXTERN_METHOD(_method) \
    jmethodID TCH_METHOD(_method);

#define TCH_REGISTER_NATIVES(_env, _class, methods) \
    if ((*_env)->RegisterNatives((JNIEnv *)_env, TCH_CLASS(_class), methods, TCH_NELEM(methods)) < 0) { \
        TCH_LOGE("RegisterNatives failed for %s\n", TCH_str(_class)); \
    }

#define TCH_FIND_CLASS(_env, _class, _className) \
    jclass TCH_LOCAL_CLASS(_class) = (*_env)->FindClass((JNIEnv *)_env, _className); \
    if (TCH_LOCAL_CLASS(_class) == NULL) { \
        TCH_LOGE("Unable to find class %s\n", _className); \
    } else { \
        TCH_CLASS(_class) = (jclass)(*_env)->NewGlobalRef((JNIEnv *)_env, TCH_LOCAL_CLASS(_class)); \
    }

#define TCH_FIND_STATIC_METHOD(_env, _class, _method, _prototype) \
    TCH_METHOD(_method) = (*_env)->GetStaticMethodID((JNIEnv *)_env, TCH_CLASS(_class), TCH_str(_method), _prototype); \
    if (TCH_METHOD(_method) == NULL) { \
        TCH_LOGE("Unable to find method %s", TCH_str(_method)); \
    }

#define TCH_FIND_STATIC_METHOD2(_env, _class, _method, _symbol, _prototype) \
    TCH_METHOD(_method) = (*_env)->GetStaticMethodID((JNIEnv *)_env, TCH_CLASS(_class), TCH_str(_symbol), _prototype); \
    if (TCH_METHOD(_method) == NULL) { \
        TCH_LOGE("Unable to find method %s", TCH_str(_method)); \
    }

#define TCH_CALL_STATIC_METHOD_ARGS(_env, _class, _method, ...) \
    (*_env)->CallStaticVoidMethod((JNIEnv *)_env, TCH_CLASS(_class), TCH_METHOD(_method), __VA_ARGS__);

#define TCH_CALL_STATIC_METHOD_BOOL(_env, _class, _method) \
    (*_env)->CallStaticBooleanMethod((JNIEnv *)_env, TCH_CLASS(_class), TCH_METHOD(_method));

TCH_REGISTER_CLASS(testclassClass);
TCH_REGISTER_METHOD(testclassConstructor);
static jobject TCH_testclassInstance = NULL;

TCH_REGISTER_CLASS(testclassNativeListenerClass);
TCH_REGISTER_METHOD(testclassNativeListenerConstructor);
static jobject TCH_testclassNativeListenerInstance = NULL;

TCH_REGISTER_METHOD(isSupported);
TCH_REGISTER_METHOD(initTestClass);

static void TCH_preload(C_JNIEnv *env) {
    TCH_ELOG;

    if (TCH_CLASS(testclassClass) == NULL) {
        TCH_FIND_CLASS(env, testclassClass, kTCHTestClassClass);
    }

    // Class really not found or not loaded, bail
    if (TCH_CLASS(testclassClass) == NULL) {
        return;
    }

    if (TCH_CLASS(testclassClass) != NULL && TCH_METHOD(isSupported) == NULL) {
        TCH_FIND_STATIC_METHOD(env, testclassClass, isSupported, "()Z");
        TCH_FIND_STATIC_METHOD(env, testclassClass, initTestClass, "(Lcom/testclass/TestClass/ITestClassListener;Landroid/app/Activity;)Z");

        if (TCH_CLASS(testclassClass) != NULL) {
            TCH_METHOD(testclassConstructor) = (*env)->GetMethodID((JNIEnv *)env, TCH_CLASS(testclassClass), "<init>", "()V");
            jobject constructor = (*env)->NewObject((JNIEnv *)env, TCH_CLASS(testclassClass), TCH_METHOD(testclassConstructor));
            TCH_testclassInstance = (*env)->NewGlobalRef((JNIEnv *)env, constructor);
        }

        if (TCH_CLASS(testclassNativeListenerClass) == NULL) {
            TCH_FIND_CLASS(env, testclassNativeListenerClass, kTCHTestClassNativeListenerClass);
        }

        if (TCH_CLASS(testclassNativeListenerClass) != NULL) {
            TCH_METHOD(testclassNativeListenerConstructor) = (*env)->GetMethodID((JNIEnv *)env, TCH_CLASS(testclassNativeListenerClass), "<init>", "()V");
            jobject listener = (*env)->NewObject((JNIEnv *)env, TCH_CLASS(testclassNativeListenerClass), TCH_METHOD(testclassNativeListenerConstructor));
            TCH_testclassNativeListenerInstance = (*env)->NewGlobalRef((JNIEnv *)env, listener);
            TCH_REGISTER_NATIVES(env, testclassNativeListenerClass, TCH_listenerMethods);
            TCH_LOGV("Initializing built-in listener");
        }
    }
}

#elif defined(__APPLE__)

#ifndef NDEBUG
#define TCH_LOGV(...)  NSLog(__VA_ARGS__)
#else
#define TCH_LOGV(...)
#endif

#define TCH_ELOG TCH_LOGV(@"%s", __FUNCTION__)

#if defined(__cplusplus)
#define TCH_NS(_class) ::_class
#else
#define TCH_NS(_class) _class
#endif

@interface TCH_delegateHandler : NSObject <TestClassDelegate>
@end

@implementation TCH_delegateHandler
- (void)testclassUploadDidProgress:(NSNumber *)videoId progress:(NSNumber *)progress {
    TCH_ELOG;
}

@end

static TCH_delegateHandler *TCH_builtInHandler = nil;

#endif /* if defined(__ANDROID__) */

#if defined(__ANDROID__)

void TCH_initTestClass(JNIEnv *env, jobject activity, jobject listener) {
    TCH_ELOG;

    C_JNIEnv *cenv = NULL;
#if defined(__cplusplus)
    cenv = (C_JNIEnv *)env;
#else
    cenv = env;
#endif

    if (TCH_vm == NULL) {
        int status = (*cenv)->GetJavaVM((JNIEnv *)cenv, &TCH_vm);
        if (status != 0) {
            TCH_LOGE("GetJavaVM failed");
            return;
        }
    }

    TCH_CLASS(testclassClass) = TCH_loadClass(cenv, activity, kTCHTestClassLoader);
    TCH_CLASS(testclassNativeListenerClass) = TCH_loadClass(cenv, activity, kTCHTestClassNativeListenerLoader);
    TCH_preload(cenv);

    jobject listenerRef = listener;
    if (listenerRef == NULL) {
        TCH_LOGV("Using built-in listener");
        listenerRef = TCH_testclassNativeListenerInstance;
    }

    TCH_CALL_STATIC_METHOD_ARGS(cenv, testclassClass, initTestClass, listenerRef, activity);
}

#elif defined(__APPLE__)

void TCH_initTestClass(UIViewController *viewController, id <TestClassDelegate>testclassDelegate) {
    TCH_ELOG;

    id <TestClassDelegate>testclassDelegateRef = testclassDelegate;
    if (testclassDelegateRef == nil) {
        TCH_builtInHandler = [[TCH_delegateHandler alloc] init];
        testclassDelegateRef = TCH_builtInHandler;
    }

    [TCH_NS(TestClass) initWithDelegate : testclassDelegateRef andParentViewController : viewController];
}

#endif /* if defined(__ANDROID__) */

bool TCH_isSupported(void) {
    TCH_ELOG;
    bool ret = false;

#if defined(__ANDROID__)
    C_JNIEnv *env = TCH_getEnv();
    if (env && TCH_CLASS(testclassClass)) {
        ret = (bool)TCH_CALL_STATIC_METHOD_BOOL(env, testclassClass, isSupported);
    }
#elif defined(__APPLE__)
    ret = (bool) [TCH_NS(TestClass) isSupported];
#endif

    return ret;
}
