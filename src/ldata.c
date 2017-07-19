#include "ldata.h"

#define LD_NIL 0	// void
#define LD_REF 1	// unsigned long (reference to a previous ld_obj)
#define LD_NUM 2	// lua_Number
#define LD_INT 3	// lua_Integer
#define LD_TRUE 4	// void
#define LD_FALSE 5	// void
#define LD_STRING 6	// size_t size; char data[size];
#define LD_FUNC 7	// size_t size; char data[size];
			// unsigned char nup; ld_sml up[nup];
#define LD_TABLE 8	// lua_Unsigned cnt; ld_sml meta; {ld_sml k, v}[cnt];
typedef unsigned char ld_type;

static int islarge(lua_State* L, int ind) {
	switch(lua_type(L, ind)) {
	case LUA_TNIL:
	case LUA_TBOOLEAN:
	case LUA_TNUMBER:
		return 0;
	case LUA_TSTRING:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
		return 1;
	default: return 0;
	}
}

// A Giant Macro that defines a particular stage of the process.
// N is the unique name of the stage.
// Macro handlers should be defined prior as _##N##_TYPE
// NIL, BOOL, NUM, INT, and REF are macro handlers for the small data types.
// STRING, TABLE, and FUNCTION are macro handlers for the large data types.

// During a particular call to stage_##N(L, ind, ud), when an object that would be
// written is found, the corrosponding macro for its type is called,
// with L and the ud as arguments. That is, as TYPE(L, ud);
// FUNCTION is slightly different, its called as FUNCTION(L, ud, p, sz),
// corrosponding to the reader interface variables. For the header, a call
// to FUNCTION_HEAD(L, ud, nups) is performed.
#define STAGE(N) \
static void _sml_##N(lua_State* L, void* ud) { \
	switch(lua_type(L, -1)) { \
	case LUA_TNIL: (_##N##_NIL(L, ud)); break; \
	case LUA_TBOOLEAN: (_##N##_BOOL(L, ud)); break; \
	case LUA_TNUMBER: \
		if(lua_isinteger) (_##N##_INT(L, ud)); \
		else (_##N##_NUM(L, ud)); \
		break; \
	case LUA_TSTRING: \
	case LUA_TTABLE: \
	case LUA_TFUNCTION: \
		(_##N##_REF(L, ud)); \
		break; \
	default: break; \
	} \
} \
static int _reader_##N(lua_State* L, const void* p, size_t sz, void* ud) { \
	(_##N##_FUNCTION(L, ud, p, sz)); \
	return 0; \
} \
static void stage_##N(lua_State* L, int tab, void* ud) { \
	tab = lua_absindex(L, tab); \
	lua_pushvalue(L, -1); \
	lua_pushboolean(L, 1); \
	lua_settable(L, tab); \
	switch(lua_type(L, -1)) { \
	case LUA_TNIL: \
	case LUA_TBOOLEAN: \
	case LUA_TNUMBER: \
		_sml_##N(L, ud); break; \
	case LUA_TSTRING: (_##N##_STRING(L, ud)); break; \
	case LUA_TTABLE: \
		if(lua_getmetatable(L, -1)) { \
			lua_gettable(L, tab); \
			if(islarge(L, -2) && !lua_toboolean(L, -1)) \
				stage_##N(L, tab, ud); \
			lua_pop(L, 1); \
		} \
		lua_pushnil(L); \
		while(lua_next(L, -2) != 0) { \
			lua_pushvalue(L, -1); \
			lua_gettable(L, tab); \
			if(islarge(L, -2) && !lua_toboolean(L, -1)) \
				stage_##N(L, tab, ud); \
			lua_pop(L, 2); \
			lua_pushvalue(L, -1); \
			lua_gettable(L, tab); \
			if(islarge(L, -2) && !lua_toboolean(L, -1)) \
				stage_##N(L, tab, ud); \
			lua_pop(L, 1); \
		} \
		(_##N##_TABLE(L, ud)); \
		if(lua_getmetatable(L, -1)) { \
			_sml_##N(L, ud); \
			lua_pop(L, 1); \
		} \
		lua_pushnil(L); \
		while(lua_next(L, -2) != 0) { \
			lua_pushvalue(L, -2); \
			_sml_##N(L, ud); \
			lua_pop(L, 1); \
			_sml_##N(L, ud); \
			lua_pop(L, 1); \
		} \
		break; \
	case LUA_TFUNCTION: { \
		lua_pushvalue(L, -1); \
		lua_Debug ar; \
		lua_getinfo(L, ">u", &ar); \
		for(int i=1; i <= ar.nups; i++) { \
			lua_getupvalue(L, -1, i); \
			lua_pushvalue(L, -1); \
			lua_gettable(L, tab); \
			if(islarge(L, -2) && !lua_toboolean(L, -1)) \
				stage_##N(L, tab, ud); \
			lua_pop(L, 1); \
		} \
		(_##N##_FUNCTION_HEAD(L, ud, ar.nups)); \
		lua_dump(L, _reader_##N, ud, 0); \
		for(int i=1; i <= ar.nups; i++) { \
			lua_getupvalue(L, -1, i); \
			_sml_##N(L, ud); \
			lua_pop(L, 1); \
		} \
		break; \
	}; \
	default: break; \
	} \
}

#define ADD(v) *(size_t*)ud += sizeof(ld_type) + (v)
#define _getsize_NIL(L, ud) ( ADD(0) )
#define _getsize_BOOL(L, ud) _getsize_NIL(L, ud)
#define _getsize_NUM(L, ud) ( ADD(sizeof(lua_Number)) )
#define _getsize_INT(L, ud) ( ADD(sizeof(lua_Integer)) )
#define _getsize_REF(L, ud) ( ADD(sizeof(unsigned long)) )
#define _getsize_STRING(L, ud) ({ \
	size_t sz; \
	lua_tolstring(L, -1, &sz); \
	ADD(sizeof(size_t) + sz); \
})
#define _getsize_TABLE(L, ud) ( ADD(sizeof(lua_Unsigned)) )
#define _getsize_FUNCTION_HEAD(L, ud, nups) ( ADD(sizeof(unsigned char)) )
#define _getsize_FUNCTION(L, ud, p, sz) ( ADD(sz) )
STAGE(getsize)
#undef ADD

size_t ld_size(lua_State* L, int ind) {
	int top = lua_gettop(L);
	lua_newtable(L);
	lua_pushvalue(L, ind);
	size_t sz = 0;
	stage_getsize(L, -2, &sz);
	lua_pop(L, 2);
	printf("ld_size top: %d from %d\n", lua_gettop(L), top);
	return sz;
}
