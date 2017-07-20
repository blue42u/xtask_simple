#include "ldata.h"
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
		lua_pushvalue(L, -1);
		lua_gettable(L, idtab);
		char buf[1+sizeof(unsigned int)] = {LD_REF};
		*(lua_Integer*)(&buf[1]) = lua_tointeger(L, -1);
		lua_pop(L, 1);
		wf(L, buf, sizeof(buf), ud);
		return;
	default: return;
	}
}
static int write_size_reader(lua_State* L, const void* p, size_t sz, void* ud) {
	*(size_t*)ud += sz;
	return 0;
}
static void write_large(lua_State* L, int idtab, unsigned int* id,
	void* ud, lua_Writer wf) {

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
			write_large(L, idtab, id, ud, wf);
			lua_pop(L, 1);
		}
		lua_pushnil(L);
		while(lua_next(L, -2) != 0) {
			cnt++;
			write_large(L, idtab, id, ud, wf);
			lua_pop(L, 1);
			write_large(L, idtab, id, ud, wf);
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
			write_large(L, idtab, id, ud, wf);
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
	}
	default: break;
	}
}
static inline void write_force(lua_State* L, int idtab, unsigned int* id,
	void* ud, lua_Writer wf) {
	idtab = lua_absindex(L, idtab);

	int t = lua_type(L, -1);
	if(t == LUA_TNIL || t == LUA_TNUMBER || t == LUA_TBOOLEAN) {
		lua_pushvalue(L, -1);
		lua_gettable(L, idtab);
		int done = lua_toboolean(L, -1);
		lua_pop(L, 1);
		if(done) return;

		lua_pushvalue(L, -1);
		lua_pushinteger(L, (*id)++);
		lua_settable(L, idtab);

		write_small(L, idtab, ud, wf);
	} else write_large(L, idtab, id, ud, wf);
}

static int addsz(lua_State* L, const void* p, size_t sz, void* ud) {
	*(size_t*)ud += sz;
	return 0;
}
size_t ld_size(lua_State* L, int n, unsigned int* numobj) {
	// Header: unsigned int numobj;
	// Footer: unsigned int numpush, pushids[numpush];
	size_t sz = sizeof(unsigned int)*(2+n);

	int top = lua_gettop(L);
	lua_newtable(L);	// For storing ids
	lua_pushglobaltable(L);
	lua_pushinteger(L, 0);
	lua_settable(L, -3);
	*numobj = 1;		// Next available id
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		write_force(L, -2, numobj, &sz, addsz);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	*numobj -= 1;
	printf("ld_size top: %d from %d\n", lua_gettop(L), top);

	return sz;
}

static int writeout(lua_State* L, const void* p, size_t sz, void* ud) {
	void** data = ud;
	memcpy(*data, p, sz);
	*data += sz;
	return 0;
}
void ld_pack(lua_State* L, int n, unsigned int numobj, void* space) {
	// Header: unsigned int numobj;
	// Footer: unsigned int numpush, pushids[numpush];
	writeout(L, &numobj, sizeof(unsigned int), &space);

	int top = lua_gettop(L);
	lua_newtable(L);	// For storing ids
	lua_pushglobaltable(L);
	lua_pushinteger(L, 0);
	lua_settable(L, -3);
	unsigned int id = 1;	// Next available id
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		write_force(L, -2, &id, &space, writeout);
		lua_pop(L, 1);
	}

	unsigned int tmp = n;
	writeout(L, &tmp, sizeof(unsigned int), &space);
	for(int i=1; i <= n; i++) {
		lua_pushvalue(L, i);
		lua_gettable(L, -2);
		tmp = lua_tointeger(L, -1);
		writeout(L, &tmp, sizeof(unsigned int), &space);
		lua_pop(L, 1);
	}

	lua_pop(L, 1);
	printf("ld_pack top: %d from %d\n", lua_gettop(L), top);
}