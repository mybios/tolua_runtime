//
// Created by sean on 15-11-23.
//

#include "jni_bundle.h"
#include <lualib.h>
#include <lua.h>
#include "lauxlib.h"

#define  LOG_TAG    "android_helper"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)

int luaopen__ejoysdk(lua_State* L);
int luaopen_ejoysdk_crypt(lua_State *L);
static int msghandler(lua_State *L);

JNIEXPORT jlong JNICALL
Java_com_ejoy_ejoysdk_EjoySDK_luaNewState(JNIEnv *env, jclass _type) {
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    LOGD("new lua state done");

    return (jlong)L;
}

JNIEXPORT void JNICALL
Java_com_ejoy_ejoysdk_EjoySDK_luaClose(JNIEnv *env, jclass _type, jlong luaState) {
    lua_State* L = (lua_State*)luaState;
    lua_close(L);
    LOGD("close lua state");
}

JNIEXPORT void JNICALL
Java_com_ejoy_ejoysdk_EjoySDK_luaCallback(JNIEnv *env, jclass _type,
                                        jlong luaState, jstring cb_type_, jint cbid, jstring message_, jbyteArray chunk) {
    lua_State* L = (lua_State*)luaState;
    const char *cb_type = (*env)->GetStringUTFChars(env, cb_type_, 0);
    const char *message = (*env)->GetStringUTFChars(env, message_, 0);

    int top = lua_gettop(L);
    lua_pushcfunction(L, msghandler);

    lua_getfield(L, LUA_REGISTRYINDEX, "_ejoysdk");
    int t = lua_getfield(L, -1, cb_type);
    if(t == LUA_TFUNCTION) {
        lua_pushnumber(L, cbid);
        lua_pushstring(L, message);
        if(chunk) {
            jint chunk_len = (*env)->GetArrayLength(env, chunk);
            jbyte* buf = (*env)->GetByteArrayElements(env, chunk, NULL);
            lua_pushlstring(L, (char*)buf, chunk_len);
            (*env)->ReleaseByteArrayElements(env, chunk, buf, 0);
        }else{
            lua_pushnil(L);
        }

        int ret = lua_pcall(L, 3, LUA_MULTRET, top + 1);
        if(ret != LUA_OK){
            LOGD("lua error %s", luaL_checkstring(L, -1));
        }
    } else {
        LOGD("lua callback function %s not found", cb_type);
    }
    lua_settop(L, top); //确保栈平衡

    if(cb_type)(*env)->ReleaseStringUTFChars(env, cb_type_, cb_type);
    if(message)(*env)->ReleaseStringUTFChars(env, message_, message);
}

JNIEXPORT jstring JNICALL
Java_com_ejoy_ejoysdk_EjoySDK_nativeDoString(JNIEnv *env, jclass _type, jlong luaState,
                                                   jstring luaCode_) {
    const char *luaCode = (*env)->GetStringUTFChars(env, luaCode_, 0);

    lua_State* L = (lua_State*)luaState;

    lua_pushcfunction(L, msghandler);
    int status = luaL_loadstring(L, luaCode);
    if (status == LUA_OK) {
        status = lua_pcall(L, 0, LUA_MULTRET, 1);
    }
    
    const char* ret = "";

    if (status != LUA_OK) {
        ret = lua_tostring(L, -1);
    }
    lua_settop(L, 0);

    (*env)->ReleaseStringUTFChars(env, luaCode_, luaCode);
    return (*env)->NewStringUTF(env, ret);
}

JNIEXPORT jlong JNICALL
Java_com_ejoy_ejoysdk_EjoySDK_ejoySDKinit(JNIEnv *env, jclass _type, jlong luaState, jobject mgr) {
    //保存全局JVM以便在子线程中使用
    JavaVM *jvm;
    (*env)->GetJavaVM(env,&jvm);
    ejoysdk_init(jvm);

    lua_State* L = (lua_State*)luaState;

    lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
    lua_State *mL = lua_tothread(L, -1);
    lua_pop(L, 1);

    LOGD("lua main thread %p -> %p", L, mL);

    luaL_requiref(mL, "_ejoysdk", luaopen__ejoysdk, 1);
    lua_pop(mL, 1);
    luaL_requiref(mL, "_ejoysdk_crypt", luaopen_ejoysdk_crypt, 1);
    lua_pop(mL, 1);

    lua_newtable(mL);
    lua_setfield(mL, LUA_REGISTRYINDEX, "_ejoysdk");

    ejoysdk_set_asset_manager(AAssetManager_fromJava(env, mgr));
    return (jlong)mL;
}


/*
** Message handler used to run all chunks
*/
static int msghandler (lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {  /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
      return 1;  /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                               luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
  return 1;  /* return the traceback */
}
