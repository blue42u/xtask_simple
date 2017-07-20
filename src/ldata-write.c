#include "lua.h"
#include <string.h>

#define LD(N) ((char[]){LD_##N})

static void write_small(lua_State* L, int idtab, void* ud,lua_Writer wf) {
	idtab = lua_absindex(L, idtab);
	switch(lua_type(L, -1)) {
	case LUA_TNIL: wf(L, LD(NIL), 1, ud); return;
	case LUA_TBOOLEAN:
		wf(L, lua_toboolean(L,-1) ? LD(TRUE) : LD(FALSE), 1, ud);
		return;
	case LUA_TNUMBER: {
		if(lua_isinteger(L, -1)) {
			char buf[1+sizeof(lua_Integer)] = {LD_INT};
			*(lua_Integer*)(&buf[1]) = lua_tointeger(L, -1);
			wf(L, buf, sizeof(buf), ud);
		} else {
			char buf[1+sizeof(lua_Number)] = {LD_NUM};
			*(lua_Number*)(&buf[1]) = lua_tonumber(L, -1);
			wf(L, buf, sizeof(buf), ud);
		}
		return;
	}
	case LUA_TSTRING:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
		if(lua_type(L, -1) != LUA_TUSERDATA
			|| luaL_testudata(L, -1, "xtask_Deferred")) {

			lua_pushvalue(L, -1);
			lua_gettable(L, idtab);
			char buf[1+sizeof(unsigned int)] = {LD_REF};
			*(lua_Integer*)(&buf[1]) = lua_tointeger(L, -1);
			lua_pop(L, 1);
			wf(L, buf, sizeof(buf), ud);
			return;
		}
	default: luaL_error(L, "unsupported type for packing: %s",
		lua_typename(L, lua_type(L, -1)));
	}
}
static int write_size_reader(lua_State* L, const void* p, size_t sz, void* ud) {
	*(size_t*)ud += sz;
	return 0;
}
static void write_large(lua_State* L, int idtab, unsigned int* id, ltask** sub,
	void* ud, lua_Writer wf, void** data) {

	luaL_checkstack(L, 3, "write_large");

	idtab = lua_absindex(L, idtab);
	int t = lua_type(L, -1);
	if(t == LUA_TNIL || t == LUA_TBOOLEAN || t == LUA_TNUMBER) return;
	lua_pushvalue(L, -1);
	lua_gettable(L, idtab);
	int done = lua_toboolean(L, -1);
	lua_pop(L, 1);
	if(done) return;

	lua_pushvalue(L, -1);
	lua_pushboolean(L, 1);
	lua_settable(L, idtab);

	lua_Unsigned cnt = 0;
	size_t size = 0;
	switch(t) {
	case LUA_TTABLE:
		if(lua_getmetatable(L, -1)) {
			write_large(L, idtab, id, sub, ud, wf, data);
			lua_pop(L, 1);
		}
		lua_pushnil(L);
		while(lua_next(L, -2) != 0) {
			cnt++;
			write_large(L, idtab, id, sub, ud, wf, data);
			lua_pop(L, 1);
			write_large(L, idtab, id, sub, ud, wf, data);
		}
		break;
	case LUA_TFUNCTION: {
		if(lua_iscfunction(L, -1))
			luaL_error(L, "can't pack C functions!");
		lua_Debug ar;
		lua_pushvalue(L, -1);
		lua_getinfo(L, ">u", &ar);
		cnt = ar.nups;
		for(int i=1; i<=ar.nups; i++) {
			lua_getupvalue(L, -1, i);
			write_large(L, idtab, id, sub, ud, wf, data);
			lua_pop(L, 1);
		}
		lua_dump(L, write_size_reader, &size, 0);
		break;
	};
	default: break;
	}

	lua_pushvalue(L, -1);
	lua_pushinteger(L, (*id)++);
	lua_settable(L, idtab);

	switch(t) {
	case LUA_TSTRING: {
		size_t sz;
		const char* s = lua_tolstring(L, -1, &sz);
		wf(L, LD(STRING), 1, ud);
		wf(L, &sz, sizeof sz, ud);
		wf(L, s, sz, ud);
		break;
	}
	case LUA_TTABLE:
		wf(L, LD(TABLE), 1, ud);
		wf(L, &cnt, sizeof(lua_Unsigned), ud);

		if(!lua_getmetatable(L, -1)) lua_pushnil(L);
		write_small(L, idtab, ud, wf);
		lua_pop(L, 1);

		lua_pushnil(L);
		while(lua_next(L, -2) != 0) {
			lua_pushvalue(L, -2);	// Key first
			write_small(L, idtab, ud, wf);
			lua_pop(L, 1);
			write_small(L, idtab, ud, wf);
			lua_pop(L, 1);
		}
		break;
	case LUA_TFUNCTION: {
		wf(L, LD(FUNC), 1, ud);
		wf(L, &size, sizeof(size_t), ud);
		lua_dump(L, wf, ud, 0);

		unsigned char nup = cnt;
		wf(L, &nup, 1, ud);
		for(int i=1; i<=nup; i++) {
			lua_getupvalue(L, -1, i);
			write_small(L, idtab, ud, wf);
			lua_pop(L, 1);
		}
		break;
	}
	case LUA_TUSERDATA: {
		ltask* lt = *(ltask**)luaL_testudata(L, -1, "xtask_Deferred");
		if(lt) {
			wf(L, LD(SUB), 1, ud);
			if(sub) {
				lt->out = *data;
				lt->task.sibling = *sub;
				*sub = lt;
			}
			void* x = NULL;
			wf(L, &x, sizeof(void*), ud);
			break;
		}
	}
	default: luaL_error(L, "unsupported type for packing: %s",
		lua_typename(L, lua_type(L, -1)));
	}
}

static int addsz(lua_State* L, const void* p, size_t sz, void* ud) {
	*(size_t*)ud += sz;
	return 0;
}
size_t ld_size(lua_State* L, int n, unsigned int* numobj) {
	// Header: unsigned int numobj;
	// Footer: unsigned int numpush; ld_sml pushes[numpush];
	size_t sz = sizeof(unsigned int)*2;

	int top = lua_gettop(L);
	lua_newtable(L);	// For storing ids
	lua_pushglobaltable(L);
	lua_pushinteger(L, 0);
	lua_settable(L, -3);
	*numobj = 1;		// Next available id
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		write_large(L, -2, numobj, NULL, &sz, addsz, NULL);
		write_small(L, -2, &sz, addsz);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	*numobj -= 1;
	if(lua_gettop(L) != top)
		luaL_error(L, "ld_size top: %d != %d\n", lua_gettop(L), top);

	return sz;
}

static int writeout(lua_State* L, const void* p, size_t sz, void* ud) {
	void** data = ud;
	memcpy(*data, p, sz);
	*data += sz;
	return 0;
}
void ld_pack(lua_State* L, int n, unsigned int numobj, void* space, ltask** sub) {
	// Header: unsigned int numobj;
	// Footer: unsigned int numpush; ld_sml pushes[numpush];
	writeout(L, &numobj, sizeof(unsigned int), &space);

	int top = lua_gettop(L);
	lua_newtable(L);	// For storing ids
	lua_pushglobaltable(L);
	lua_pushinteger(L, 0);
	lua_settable(L, -3);
	unsigned int id = 1;	// Next available id
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		write_large(L, -2, &id, sub, &space, writeout, &space);
		lua_pop(L, 1);
	}

	unsigned int tmp = n;
	writeout(L, &tmp, sizeof(unsigned int), &space);
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		write_small(L, -2, &space, writeout);
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	if(lua_gettop(L) != top)
		luaL_error(L, "ld_pack top: %d != %d\n", lua_gettop(L), top);
}
