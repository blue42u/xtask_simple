#include "xtask.h"
#include "lua.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static int msgh(lua_State* L) {
	luaL_traceback(L, L, lua_tostring(L, -1), 0);
	return 1;
}

static void* ltaskfunc(void* state, void* vlt) {
	lua_State* L = state;
	ltask* lt = vlt;

	lua_settop(L, 0);
	lua_pushcfunction(L, msgh);
	int nargs = ld_unpack(L, lt->pack)-1;
	lua_pcall(L, nargs, LUA_MULTRET, 1);
	lua_remove(L, 1);

	unsigned int nobj;
	ltask* tail = NULL;
	*lt->out = malloc(ld_size(L, lua_gettop(L), &nobj));
	ld_pack(L, lua_gettop(L), nobj, *lt->out, &tail);

	free(lt);
	return tail;
}

// .call([<fate>], <function>, <args|Deferred>...) -> Deferred
// Builds a new Deferred task, using the given arguments as a base.
static int l_call(lua_State* L) {
	// Process the fate argument, to get it out of the way
	int fate = 0;
	const char* fatestr = lua_isstring(L, 1) ? lua_tostring(L, 1) : NULL;
	if(fatestr) {
		if(strchr(fatestr, 'l')) fate |= XTASK_FATE_LEAF;
		lua_remove(L, 1);
	}

	// Allocate and fill the new task
	unsigned int nobj;
	ltask* lt = malloc(sizeof(ltask) + ld_size(L, lua_gettop(L), &nobj));
	ltask* children = NULL;
	ld_pack(L, lua_gettop(L), nobj, lt->pack, &children);
	lt->task = (xtask_task){ ltaskfunc, fate, NULL, children };

	// Create the resulting Deferred, and return it
	*(ltask**)lua_newuserdata(L, sizeof(ltask*)) = lt;
	luaL_setmetatable(L, "xtask_Deferred");
	return 1;
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

// Config := {
//	[workers = <integer > 0>,]
//	[max_leafing = <integer>,]
//	[max_tailing = <integer>,]
//	[init = <function>,]
//	[dest = <function>,]
// }

// .run(<Config>, <Deferred>) -> <retvalue>...
// Runs a Deferred task from outside the system. Inside should not use this.
static int l_run(lua_State* L) {
	lua_settop(L, 2);

	xtask_config xc = {.workers=3, .init=init, .dest=dest};

	ltask* lt = *(ltask**)luaL_checkudata(L, 2, "xtask_Deferred");
	void* out = NULL;
	lt->out = &out;
	xtask_run(lt, xc);

	return ld_unpack(L, out);
}

luaL_Reg funcs[] = {
	{"call", l_call},
	{"run", l_run},
	{NULL, NULL}
};

int luaopen_xtask(lua_State* L) {
	luaL_newmetatable(L, "xtask_Deferred");
	luaL_newlib(L, funcs);
	return 1;
}
