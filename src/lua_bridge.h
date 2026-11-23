#ifndef LUA_BRIDGE_H
#define LUA_BRIDGE_H

#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include <stdbool.h>

// Registers all engine C functions into the Lua runtime environment
void lua_bridge_register(lua_State* L);

// Polls the active DualShock 3 controller
void lua_bridge_poll_pad(void);

// Returns whether exit has been requested (e.g. START + SELECT or PS Button)
bool lua_bridge_should_exit(void);

#endif // LUA_BRIDGE_H
