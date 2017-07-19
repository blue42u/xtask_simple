#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
#include <lua5.3/lauxlib.h>

#ifndef _LDATA_H_
#define _LDATA_H_

// Calculate how much memory it would take to pack the bottom <n> objects.
size_t ld_size(lua_State* L, int n);

// Serialize the bottom n objects, and pack it into <space>, with enough space.
// Returns the amount of data actually written.
size_t ld_pack(lua_State* L, int n, void* space);

// Deserialize a pack, pushing the results in the same order onto the stack.
// Returns the number of objects unpacked.
//int ld_unpack(lua_State* L, void* space);

#endif
