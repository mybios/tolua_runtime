#ifndef EJOYSDK_JNI_BUNDLE_H
#define EJOYSDK_JNI_BUNDLE_H

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#define  LOG_TAG    "android_helper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

typedef struct {
    JNIEnv*    env;
    jclass     class_id;
    jmethodID  method_id;
} JniMethodInfo;

void ejoysdk_set_asset_manager(AAssetManager* mgr);
AAssetManager* ejoysdk_get_asset_manager();

int ejoysdk_get_static_method_info(JniMethodInfo *methodinfo, const char *className,
                                   const char *methodName, const char *paramCode);
int ejoysdk_get_method_info(JniMethodInfo* methodinfo, const char *className,
                            const char *methodName, const char *paramCode);
void ejoysdk_init(JavaVM *vm);

#endif //EJOYSDK_JNI_BUNDLE_H
