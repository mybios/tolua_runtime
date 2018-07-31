//
// Created by sean on 15-11-23.
//

#include "jni_bundle.h"
#include <android/log.h>
#include <dlfcn.h>

#define  LOG_TAG    "android_helper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

static JavaVM* _ejoysdk_javavm = NULL;
static AAssetManager* _ejoysdk_assert_mgr= NULL;

void ejoysdk_init(JavaVM *vm) {
    _ejoysdk_javavm = vm;
}

static inline JavaVM *get_javavm() {
    return _ejoysdk_javavm;
}

void ejoysdk_set_asset_manager(AAssetManager *mgr) {
    _ejoysdk_assert_mgr = mgr;
}

AAssetManager *ejoysdk_get_asset_manager() {
   return _ejoysdk_assert_mgr;
}

static int get_java_env(JNIEnv **env) {
    JavaVM* jvm = get_javavm();

    if ((*jvm)->GetEnv(jvm, (void **) env, JNI_VERSION_1_4) != JNI_OK) {
        LOGD("Failed to get the environment using GetEnv()");
        return -1;
    }

    if ((*jvm)->AttachCurrentThread(jvm, env, 0) < 0) {
        LOGD("Failed to get the environment using AttachCurrentThread()");
        return -1;
    }

    return 0;
}

static jclass get_class_id(const char *className, JNIEnv *env) {
    JNIEnv *pEnv = env;
    jclass ret = 0;

    do {
        if (!pEnv) {
            if (get_java_env(&pEnv) < 0)
                break;
        }

        ret = (*pEnv)->FindClass(pEnv, className);
        if (!ret) {
            LOGD("Failed to find class of %s", className);
            break;
        }
    } while (0);

    return ret;
}

int ejoysdk_get_static_method_info(JniMethodInfo *methodinfo, const char *className,
                           const char *methodName, const char *paramCode) {
    jmethodID method_id = 0;
    JNIEnv *pEnv = 0;
    int bRet = -1;

    do {
        if (get_java_env(&pEnv) < 0)
            break;

        jclass class_id = get_class_id(className, pEnv);

        method_id = (*pEnv)->GetStaticMethodID(pEnv, class_id, methodName, paramCode);
        if (!method_id) {
            LOGD("Failed to find static method id of %s", methodName);
            break;
        }

        methodinfo->class_id = class_id;
        methodinfo->env = pEnv;
        methodinfo->method_id = method_id;

        bRet = 0;
    } while (0);

    return bRet;
}

int ejoysdk_get_method_info(JniMethodInfo *methodinfo, const char *className, const char *methodName,
                    const char *paramCode) {
    jmethodID method_id = 0;
    JNIEnv *pEnv = 0;
    int bRet = -1;

    do {
        if (get_java_env(&pEnv) < 0)
            break;

        jclass class_id = get_class_id(className, pEnv);

        method_id = (*pEnv)->GetMethodID(pEnv, class_id, methodName, paramCode);
        if (!method_id) {
            LOGD("Failed to find method id of %s", methodName);
            break;
        }

        methodinfo->class_id = class_id;
        methodinfo->env = pEnv;
        methodinfo->method_id = method_id;

        bRet = 0;
    } while (0);

    return bRet;
}
