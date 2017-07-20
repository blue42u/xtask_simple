#include "xtask.h"
#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
#include <lua5.3/lauxlib.h>

#ifndef _LUA_H_
#define _LUA_H_

typedef struct {
	xtask_task task;
	void** out;	// Where to put the packed output
	char pack[];	// Packed data, filled by ld_pack
} ltask;

#define LD_NIL 'n'	// void
#define LD_REF 'r'	// unsigned int (index of another obj, less than this one)
#define LD_NUM 'm'	// lua_Number
#define LD_INT 'i'	// lua_Integer
#define LD_TRUE 't'	// void
#define LD_FALSE 'f'	// void
#define LD_STRING 'S'	// size_t size; char data[size];
#define LD_FUNC 'F'	// size_t size; char data[size];
			// unsigned char nup; ld_sml up[nup];
#define LD_TABLE 'T'	// lua_Unsigned cnt; ld_sml meta; {ld_sml k, v}[cnt];
#define LD_SUB 'b'	// void* subpack;
typedef unsigned char ld_type;

// Calculate how much memory it would take to pack the bottom <n> objects.
// Returns the size and (by pointer) the number of objects in the bundle.
size_t ld_size(lua_State* L, int n, unsigned int* numobj);

// Serialize the bottom n objects, and pack it into <space>, with enough space.
// The Deferred's in this pack will form a linked list (using sibling) at *sub.
void ld_pack(lua_State* L, int n, unsigned int numobj, void* space, ltask** sub);

// Deserialize a pack, pushing the results in the same order onto the stack.
// Returns the number of objects unpacked.
int ld_unpack(lua_State* L, const void* space);

#endif
