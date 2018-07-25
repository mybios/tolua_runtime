
#include <string.h>
#if !defined __APPLE__
#include <malloc.h>
#endif
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "tolua.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#include <sys/time.h>
#endif

/*
** {==============================================================
** some useful macros
** ===============================================================
*/

#undef lua_tonumber
LUALIB_API lua_Number lua_tonumber(lua_State *L, int idx)
{
	return lua_tonumberx(L, idx, NULL);
}

#undef lua_tointeger
LUALIB_API lua_Integer lua_tointeger(lua_State *L, int idx)
{
	return lua_tointegerx(L, idx, NULL);
}

#undef lua_pop
LUALIB_API void lua_pop(lua_State *L, int n)
{
	lua_settop(L, -(n)-1);
}

#undef lua_newtable
LUALIB_API void lua_newtable(lua_State *L)
{
	lua_createtable(L, 0, 0);
}


#undef lua_pushcfunction
LUALIB_API void lua_pushcfunction(lua_State *L, lua_CFunction fn)
{
	lua_pushcclosure(L, fn, 0);
}



#undef lua_register
LUALIB_API void lua_register(lua_State *L, const char *name, lua_CFunction fn)
{
	lua_pushcfunction(L, fn);
	lua_setglobal(L, name);
}


#undef lua_isfunction
LUALIB_API int lua_isfunction(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TFUNCTION;
}


#undef lua_istable
LUALIB_API int lua_istable(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TTABLE;
}


#undef lua_islightuserdata
LUALIB_API int lua_islightuserdata(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TLIGHTUSERDATA;
}


#undef lua_isnil
LUALIB_API int lua_isnil(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TNIL;
}


#undef lua_isboolean
LUALIB_API int lua_isboolean(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TBOOLEAN;
}


#undef lua_isthread
LUALIB_API int lua_isthread(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TTHREAD;
}


#undef lua_isnone
LUALIB_API int lua_isnone(lua_State *L, int n)
{
	return lua_type(L, (n)) == LUA_TNONE;
}


#undef lua_isnoneornil
LUALIB_API int lua_isnoneornil(lua_State *L, int n)
{
	return lua_type(L, (n)) <= 0;
}

#undef lua_pushliteral
LUALIB_API const char * lua_pushliteral(lua_State *L, const char *s)
{
	return lua_pushstring(L, s);
}

#undef lua_pushglobaltable
LUALIB_API int lua_pushglobaltable(lua_State *L)
{
	return lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);
}

#undef lua_tostring
LUALIB_API const char * lua_tostring(lua_State *L, int i)
{
	return lua_tolstring(L, i, NULL);
}


#undef lua_insert
LUALIB_API void lua_insert(lua_State *L, int idx)
{
	lua_rotate(L, (idx), 1);
}

#undef lua_remove
LUALIB_API void lua_remove(lua_State *L, int idx)
{
	lua_rotate(L, (idx), -1);
	lua_pop(L, 1);
}

#undef lua_replace
LUALIB_API void lua_replace(lua_State *L, int idx)
{
	lua_copy(L, -1, (idx));
	lua_pop(L, 1);
}


#undef lua_newuserdata
LUALIB_API void lua_newuserdata(lua_State *L, size_t sz)
{
	lua_newuserdatauv(L, sz, 1);
}


#undef lua_getuservalue
LUALIB_API void lua_getuservalue(lua_State *L, int idx)
{
	lua_getiuservalue(L, idx, 1);
}


#undef lua_setuservalue
LUALIB_API void lua_setuservalue(lua_State *L, int idx)
{
	lua_setiuservalue(L, idx, 1);
}


#undef lua_call
LUALIB_API void lua_call(lua_State *L, int nargs, int nresults)
{
	lua_callk(L, nargs, nresults, 0, NULL);
}

#undef lua_pcall
LUALIB_API void lua_pcall(lua_State *L, int nargs, int nresults, int errfunc)
{
	lua_pcallk(L, nargs, nresults, errfunc, 0, NULL);
}

#undef lua_yield
LUALIB_API void lua_yield(lua_State *L, int idx)
{
	lua_yieldk(L, idx, 0, NULL);
}


LUALIB_API const char *luaL_findtable(lua_State *L, int idx,
	const char *fname, int szhint) {
	const char *e;
	if (idx) lua_pushvalue(L, idx);
	do {
		e = strchr(fname, '.');
		if (e == NULL) e = fname + strlen(fname);
		lua_pushlstring(L, fname, e - fname);
		if (lua_rawget(L, -2) == LUA_TNIL) {  /* no such field? */
			lua_pop(L, 1);  /* remove this nil */
			lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
			lua_pushlstring(L, fname, e - fname);
			lua_pushvalue(L, -2);
			lua_settable(L, -4);  /* set new table into field */
		}
		else if (!lua_istable(L, -1)) {  /* field has a non-table value? */
			lua_pop(L, 2);  /* remove table and value */
			return fname;  /* return problematic part of the name */
		}
		lua_remove(L, -2);  /* remove previous table */
		fname = e + 1;
	} while (*e == '.');
	return NULL;
}