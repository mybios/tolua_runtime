#include <lua.h>
#include "jni_bundle.h"
#include "lua.h"
#include "lauxlib.h"
#include "string.h"
#include <stdlib.h>

#define EJ_PACKAGE "com/ejoy/ejoysdk/"

static int _log(lua_State* L) {
	JniMethodInfo mi;
	if (ejoysdk_get_static_method_info(&mi, EJ_PACKAGE"EjoySDK", "output", "(Ljava/lang/String;)V") < 0) {
		lua_pushnumber(L, -1);
		return 1;
	}

	const char* log = lua_tostring(L, -1);
	jstring logStr = (*mi.env)->NewStringUTF(mi.env, log);

	(*mi.env)->CallStaticVoidMethod(mi.env, mi.class_id, mi.method_id, logStr);
	(*mi.env)->DeleteLocalRef(mi.env, logStr);
	(*mi.env)->DeleteLocalRef(mi.env, mi.class_id);

	lua_pushnumber(L, 0);
    return 1;
}

static unsigned char*
read_asset_file(const char* filename, unsigned long* size) {
	unsigned char* data = 0;
	do {
		/*获取文件名并打开*/
        AAssetManager *mgr = ejoysdk_get_asset_manager();
		AAsset* asset = AAssetManager_open(mgr, filename, AASSET_MODE_UNKNOWN);
		if(asset == NULL) {
			LOGD("getFileData file not exist %s", filename);
			return NULL;
		}

		/*获取文件大小*/
		off_t bufferSize = AAsset_getLength(asset);
		data = (unsigned char *)malloc(bufferSize);
		int numBytesRead = AAsset_read(asset, data, bufferSize);
		*size = bufferSize;

		/*关闭文件*/
		AAsset_close(asset);

	} while (0);
	return data;
}

static unsigned char* read_file(const char* filename,
						   const char* mode,
						   unsigned long * size) {
	unsigned char* data = 0;

	if((!filename)||(!mode)) {
		return NULL;
	}

	if(filename[0] != '/') {
		data = read_asset_file(filename, size);
	} else {
		do {
			// read rrom other path than user set it
			FILE *fp = fopen(filename, mode);
			if(!fp) break;

			unsigned long sz;
			fseek(fp,0,SEEK_END);
			sz = ftell(fp);
			fseek(fp,0,SEEK_SET);
			data = (unsigned char*) malloc(sz);
			if(!data) {
				LOGD("FAILE to load data %s",filename);
				return NULL;
			}
			sz = fread(data,sizeof(unsigned char), sz,fp);
			fclose(fp);

			if (size) {
				*size = sz;
			}
		} while (0);
	}

	return data;
}

static int _lread(lua_State* L) {
	const char* filename = luaL_checkstring(L, 1);

	unsigned long size;
	char* data = (char*)read_file(filename, "rb", &size);
	if(data) {
		lua_pushlstring(L, data, size);
		free(data);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

static int _async_call(lua_State* L) {
    const char* cls = luaL_checkstring(L, 1);
    const char* call_ = luaL_checkstring(L, 2);
    int cbid = luaL_checkinteger(L, 3);
    const char* params_ = luaL_checkstring(L, 4);
    size_t chunk_len;
    const char* chunk_ = luaL_checklstring(L, 5, &chunk_len);

    JniMethodInfo lcall;
    if (ejoysdk_get_static_method_info(&lcall, cls, "async_call",
                               "(Ljava/lang/String;ILjava/lang/String;[B)V") < 0) {
        lua_pushnumber(L, -1);
        return 1;
    }

    JNIEnv* env = lcall.env;
    jstring call = (*env)->NewStringUTF(env, call_);
    jstring params = (*env)->NewStringUTF(env, params_);
    jbyteArray chunk = (*env)->NewByteArray(env, chunk_len);
    if(chunk_len) {
        (*env)->SetByteArrayRegion(env, chunk, 0, chunk_len, (jbyte*)chunk_);
    }

    (*env)->CallStaticVoidMethod(lcall.env, lcall.class_id, lcall.method_id, call,
                                   cbid, params, chunk);
    (*env)->DeleteLocalRef(env, chunk);
    (*env)->DeleteLocalRef(env, params);
    (*env)->DeleteLocalRef(env, lcall.class_id);
    (*env)->DeleteLocalRef(env, call);

    lua_pushnumber(L, cbid);
    return 1;
}

static int _sync_call(lua_State* L) {
    const char* cls = luaL_checkstring(L, 1);
    const char* call_ = luaL_checkstring(L, 2);
    const char* params_ = luaL_checkstring(L, 3);
    size_t chunk_len;
    const char* chunk_ = luaL_checklstring(L, 4, &chunk_len);

    JniMethodInfo lcall;
    if (ejoysdk_get_static_method_info(&lcall, cls, "sync_call",
                               "(Ljava/lang/String;Ljava/lang/String;[B)Ljava/lang/String;") < 0) {
        lua_pushnumber(L, -1);
        return 1;
    }

    JNIEnv* env = lcall.env;
    jstring call = (*env)->NewStringUTF(env, call_);
    jstring params = (*env)->NewStringUTF(env, params_);
    jbyteArray chunk = (*env)->NewByteArray(env, chunk_len);
    if(chunk_len) {
        (*env)->SetByteArrayRegion(env, chunk, 0, chunk_len, (jbyte*)chunk_);
    }

    jstring ret_ = (*env)->CallStaticObjectMethod(lcall.env, lcall.class_id, lcall.method_id, call, params, chunk);
    
    if(ret_) {
        size_t ret_len = (*env)->GetStringUTFLength(lcall.env, ret_);
        const char* ret = (*env)->GetStringUTFChars(lcall.env, ret_, NULL);
        lua_pushlstring(L, ret, ret_len);
    } else {
        lua_pushnil(L);
    }


    (*env)->DeleteLocalRef(env, lcall.class_id);
    (*env)->DeleteLocalRef(env, call);
    (*env)->DeleteLocalRef(env, params);
    (*env)->DeleteLocalRef(env, chunk);
    
    (*env)->DeleteLocalRef(env, ret_);

    return 1;
}

static void push_string_field(lua_State* L, JNIEnv* env, 
        jclass cls, jobject obj, const char* field_name) {
    
    jfieldID fid = (*env)->GetFieldID(env, cls, field_name, "Ljava/lang/String;");
    jstring jstr = (*env)->GetObjectField(env, obj, fid);
    if(jstr){
        size_t str_len = (*env)->GetStringUTFLength(env, jstr);
        const char* str = (*env)->GetStringUTFChars(env, jstr, NULL);
        lua_pushlstring(L, str, str_len);
        (*env)->ReleaseStringUTFChars(env, jstr, str);
        (*env)->DeleteLocalRef(env, jstr);
    } else {
        lua_pushnil(L);
    }
}

static void push_int_field(lua_State* L, JNIEnv* env,
        jclass cls, jobject obj, const char* field_name) {

    jfieldID fid = (*env)->GetFieldID(env, cls, field_name, "I");
    jint cbid = (*env)->GetIntField(env, obj, fid);
    lua_pushnumber(L, cbid);
}

static void push_byte_array_field(lua_State* L, JNIEnv* env,
        jclass cls, jobject obj, const char* field_name) {

    jfieldID fid = (*env)->GetFieldID(env, cls, field_name, "[B");
    jbyteArray chunk = (*env)->GetObjectField(env, obj, fid);
    if(chunk) {
        jint chunk_len = (*env)->GetArrayLength(env, chunk);
        luaL_Buffer b;
        char* buf = luaL_buffinitsize(L, &b, chunk_len);
        (*env)->GetByteArrayRegion(env, chunk, 0, chunk_len, (jbyte*)buf);
        luaL_pushresultsize(&b, chunk_len);

        (*env)->DeleteLocalRef(env, chunk);
    }else{
        lua_pushnil(L);
    }
}

static int _tick(lua_State* L) {
    const char* cls_name = luaL_checkstring(L, 1);

    JniMethodInfo lcall;
    if (ejoysdk_get_static_method_info(&lcall, cls_name, "tick",
                               "()Lcom/ejoy/ejoysdk/EjoySDK$LuaCallbackAction;") < 0) {
        lua_pushnumber(L, -1);
        return 1;
    }
    
    JNIEnv* env = lcall.env;
    jobject obj = (*env)->CallStaticObjectMethod(lcall.env, lcall.class_id, lcall.method_id);
    
    if(!obj){
        (*env)->DeleteLocalRef(env, lcall.class_id);
        lua_pushnil(L);
        return 1;
    };

    jclass cls = (*env)->GetObjectClass(env, obj);

    push_string_field(L, env, cls, obj, "cb_type");
    push_int_field(L, env, cls, obj, "cbid");
    push_string_field(L, env, cls, obj, "msg");
    push_byte_array_field(L, env, cls, obj, "chunk");

    (*env)->DeleteLocalRef(env, cls);
    (*env)->DeleteLocalRef(env, obj);
    (*env)->DeleteLocalRef(env, lcall.class_id);
    return 4; 
}


static int _invoke(lua_State* L) {
    const char* cls = luaL_checkstring(L, 1);
    const char* call_ = luaL_checkstring(L, 2);
    const char* params_ = luaL_checkstring(L, 3);
    size_t chunk_len;
    const char* chunk_ = luaL_checklstring(L, 4, &chunk_len);

    JniMethodInfo lcall;
    if (ejoysdk_get_static_method_info(&lcall, cls, "invoke", "(Ljava/lang/String;Ljava/lang/String;[B)V") < 0) {
        lua_pushnumber(L, -1);
        return 1;
    }

    JNIEnv* env = lcall.env;
    jstring call = (*env)->NewStringUTF(env, call_);
    jstring params = (*env)->NewStringUTF(env, params_);
    jbyteArray chunk = (*env)->NewByteArray(env, chunk_len);
    if(chunk_len) {
        (*env)->SetByteArrayRegion(env, chunk, 0, chunk_len, (jbyte*)chunk_);
    }

    (*env)->CallStaticVoidMethod(lcall.env, lcall.class_id, lcall.method_id, call, params, chunk);

    (*env)->DeleteLocalRef(env, lcall.class_id);
    (*env)->DeleteLocalRef(env, call);
    (*env)->DeleteLocalRef(env, params);
    (*env)->DeleteLocalRef(env, chunk);

    lua_pushnumber(L, 1);
    return 1;
}

static int _register_cb(lua_State* L) {
    const char* cb_type_name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    lua_getfield(L, LUA_REGISTRYINDEX, "_ejoysdk");
    lua_pushvalue(L, 2);
    lua_setfield(L, -2, cb_type_name);
    return 0;
}

static int _get_register_cb(lua_State* L) {
    const char* cb_type_name = luaL_checkstring(L, 1);
    if(cb_type_name){
        lua_getfield(L, LUA_REGISTRYINDEX, "_ejoysdk");
        lua_getfield(L, -1, cb_type_name);
        return 1;
    }
    return 0;
}

static int _os(lua_State *L) {
    lua_pushstring(L, "android");
    return 1;
}

static int _bin_version(lua_State *L) {
    lua_pushstring(L, "1.0.0");
    return 1;
}

int luaopen__ejoysdk(lua_State* L) {
    luaL_checkversion(L);

    luaL_Reg reg[] = {
            {"log", _log},
            {"lread", _lread},
            {"async_call", _async_call},
            {"sync_call", _sync_call},
            {"invoke", _invoke},
            {"register_cb", _register_cb},
            {"get_register_cb", _get_register_cb},
            {"tick", _tick},
            {"os", _os},
            {"bin_version", _bin_version},
            {NULL, NULL}
    };

    luaL_newlib(L, reg);
    return 1;
}
