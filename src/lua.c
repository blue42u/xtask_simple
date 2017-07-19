#include "xtask.h"
#include "ldata.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// TaskTree := {
//	func=<function>,
//	data=<string>,
//	fate=<stringmask>,
//	<child TaskTree>,...
// }
// TaskTree.func should return <retvalue>, <TaskTree>...
// If <retvalue> is not a true value, <TaskTree> will be the tail.
// Otherwise, <retvalue> must be a string.
// For Task Tree.fate:
//	'l' means _LEAF.

// Config := {
//	[workers = <integer > 0>,]
//	[max_leafing = <integer>,]
//	[max_tailing = <integer>,]
//	[init = <function>,]
//	[dest = <function>,]
// }

typedef struct {
	size_t l;
	char* s;
} lstr;

static inline lstr tolstr(lua_State* L, int ind) {
	lstr ls;
	const char* s = luaL_checklstring(L, ind, &ls.l);
	ls.s = malloc(ls.l);
	memcpy(ls.s, s, ls.l);
	return ls;
}

static inline void freelstr(lstr ls) {
	free(ls.s);
}

static inline int haslstr(lstr ls, int c) {
	return memchr(ls.s, c, ls.l) != NULL;
}

typedef struct {
	xtask_task t;
	lstr* out;
	lstr f;
	lstr arg;
	int numc;
	lstr cargs[];
} ltask;
static void* ltaskfunc(void*, void*);

static int writer(lua_State* L, const void* f, size_t s, void* vlt) {
	ltask* lt = vlt;
	lt->f.s = realloc(lt->f.s, lt->f.l+s);
	memcpy(lt->f.s + lt->f.l, f, s);
	lt->f.l += s;
	return 0;
}

static ltask* totask(lua_State* L, int ind) {
	ind = lua_absindex(L, ind);
	ltask* t;

	int top = lua_gettop(L);	// Begin stack-protected area
	luaL_checkstack(L, 4, "In totask, stack-protected area");
	{
		lua_len(L, ind);
		t = malloc(sizeof(ltask)+lua_tointeger(L, -1)*sizeof(lstr));
		*t = (ltask){ {ltaskfunc, 0, NULL, NULL},
			.numc = lua_tointeger(L, -1),
			.f = {0, NULL}};

		if(lua_getfield(L, ind, "func") != LUA_TFUNCTION)
			luaL_error(L, "Task.func is not a function!");
		lua_dump(L, writer, t, 0);

		if(lua_getfield(L, ind, "data") != LUA_TSTRING)
			luaL_error(L, "Task.data is not a string!");
		t->arg = tolstr(L, -1);

		if(lua_getfield(L, ind, "fate") != LUA_TSTRING)
			luaL_error(L, "Task.fate is not a string!");
		lstr fate = tolstr(L, -1);
		if(haslstr(fate, 'l')) t->t.fate |= XTASK_FATE_LEAF;
	}
	lua_settop(L, top);		// End stack-protected area

	for(int i=0; i < t->numc; i++) {
		lua_geti(L, ind, i);
		ltask* c = totask(L, -1);
		c->t.sibling = t->t.child;
		t->t.child = c;
		lua_pop(L, 1);
	}

	return t;
}

static int runtask(lua_State* L) {
	luaL_checkstack(L, 3, "In runtask");
	ltask* lt = lua_touserdata(L, 1);
	lua_settop(L, 0);

	if(luaL_loadbuffer(L, lt->f.s, lt->f.l, "Task"))
		return luaL_error(L, "Error transferring function: %s\n",
			lua_tostring(L, -1));
	lua_pushlstring(L, lt->arg.s, lt->arg.l);
	lua_call(L, 1, LUA_MULTRET);

	if(lua_toboolean(L, 1)) {
		*lt->out = tolstr(L, 1);
		return 0;
	} else {
		int top = lua_gettop(L);
		ltask* tail = NULL;
		for(int i=1; i <= top; i++) {
			ltask* l = totask(L, i);
			l->t.sibling = tail;
			tail = l;
		}
		lua_pushlightuserdata(L, tail);
		return 1;
	}
}

static int msgh(lua_State* L) {
	luaL_traceback(L, L, lua_tostring(L, -1), 0);
	return 1;
}

static void* ltaskfunc(void* state, void* vlt) {
	lua_State* L = state;
	ltask* lt = vlt;

	lua_pushcfunction(L, msgh);
	lua_pushcfunction(L, runtask);
	lua_pushlightuserdata(L, vlt);
	if(lua_pcall(L, 1, 1, -3)) {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		exit(1);
	}
	void* tail = lua_touserdata(L, -1);
	lua_pop(L, 2);

	freelstr(lt->f);
	freelstr(lt->arg);
	for(int i=0; i < lt->numc; i++) freelstr(lt->cargs[i]);
	free(lt);
	return tail;
}

static void* init() {
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	return L;
}

static void dest(void* vL) {
	lua_State* L = vL;
	lua_close(L);
}

// .run(<Config>, <TaskTree>...) -> <retvalue>...
static int l_run(lua_State* L) {
	xtask_config xc = {.workers=3, .init=init, .dest=dest};
	int top = lua_gettop(L);

	lstr outs[top-1];
	ltask* t = NULL;
	for(int i=2; i <= top; i++) {
		ltask* l = totask(L, i);
		l->out = &outs[i-2];
		l->t.sibling = t;
		t = l;
	}
	xtask_run(t, xc);

	lua_settop(L, 0);
	for(int i=0; i < top-1; i++) lua_pushlstring(L, outs[i].s, outs[i].l);
	return lua_gettop(L);
}

// .pack(...) -> <packedstring>
static int l_pack(lua_State* L) {
	size_t sz = 0;
	for(int i=1; i <= lua_gettop(L); i++) sz += ld_size(L, i);
	lua_pushinteger(L, sz);
	return 1;
}

luaL_Reg funcs[] = {
	{"run", l_run},
	{"pack", l_pack},
	//{"unpack", l_unpack},
	{NULL, NULL}
};

int luaopen_xtask(lua_State* L) {
	luaL_newlib(L, funcs);
	return 1;
}
