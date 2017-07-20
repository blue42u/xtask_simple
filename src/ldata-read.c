#include "lua.h"

static inline const void* read_size(size_t size, const void** space) {
	const void* ret = *space;
	*space += size;
	return ret;
}
#define read(T) *(T*)read_size(sizeof(T), data)
#define next(T, S) (T*)read_size(S, data)

static int pushread(lua_State* L, int idtab, const void** data,
	const void* st, int suball);

int ld_unpack(lua_State* L, const void* space) {
	// Header: unsigned int numobj;
	// Footer: unsigned int numpush; ld_sml pushes[numpush];
	int top = lua_gettop(L);
	const void* st = space;
	void* data = &space;

	int n = read(unsigned int);
	lua_createtable(L, n, 0);
	lua_pushglobaltable(L);
	lua_seti(L, -2, 0);
	for(int i=1; i<=n; i++) {
		pushread(L, -1, data, st, 0);
		lua_seti(L, -2, i);
	}
	int tab = lua_absindex(L, -1);

	n = read(unsigned int);
	int pushed = 0;
	for(int i=1; i<=n; i++) pushed += pushread(L, tab, data, st, i==n);
	lua_remove(L, tab);

	if(lua_gettop(L) != top + pushed)
		luaL_error(L, "ld_unpack top: %d != %d+%d\n", lua_gettop(L),
			top, pushed);
	return pushed;
}

static int pushread(lua_State* L, int idtab, const void** data,
	const void* st, int suball) {

	idtab = lua_absindex(L, idtab);
	ld_type t = read(ld_type);
	switch(t) {
	case LD_NIL: lua_pushnil(L); return 1;
	case LD_REF: lua_geti(L, idtab, read(unsigned int)); return 1;
	case LD_NUM: lua_pushnumber(L, read(lua_Number)); return 1;
	case LD_INT: lua_pushinteger(L, read(lua_Integer)); return 1;
	case LD_TRUE: lua_pushboolean(L, 1); return 1;
	case LD_FALSE: lua_pushboolean(L, 0); return 1;
	case LD_STRING: {
		size_t sz = read(size_t);
		lua_pushlstring(L, next(char, sz), sz);
		return 1;
	};
	case LD_FUNC: {
		size_t sz = read(size_t);
		if(luaL_loadbuffer(L, next(char, sz), sz, "Task") != LUA_OK)
			luaL_error(L, "loading function from pack!");
		int nup = read(unsigned char);
		for(int i=1; i<=nup; i++) {
			pushread(L, idtab, data, st, 0);
			lua_setupvalue(L, -2, i);
		}
		return 1;
	};
	case LD_TABLE: {
		lua_Unsigned cnt = read(lua_Unsigned);
		lua_createtable(L, 0, cnt);
		pushread(L, idtab, data, st, 0);
		lua_setmetatable(L, -2);
		for(int i=0; i<cnt; i++) {
			pushread(L, idtab, data, st, 0);
			pushread(L, idtab, data, st, 0);
			lua_settable(L, -3);
		}
		return 1;
	};
	case LD_SUB: {
		void* subdata = read(void*);
		int n = ld_unpack(L, subdata);
		if(suball) lua_pop(L, n-1);
		return suball ? n : 1;
	};
	default:
		return luaL_error(L, "unrecognized type %c (%d) at %p", t, t,
			*data - sizeof(ld_type) - st);
	}
}
