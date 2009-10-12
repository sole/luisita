#ifndef LUISITA_H
#define LUISITA_H
 
#include "my_lua.hpp"
 
extern int luisita_init();
extern int luisita_end();
extern void luisita_run();
extern int luisita_loadScript(const char *scriptFile);
extern int luisita_hasFinished();
extern void luisita_render();
 
void luisita_reportErrors(lua_State *L, int status);

// Functions which are made available to Lua
// ---------------------------------------

// logging functions
static int luisita_luaTrace(lua_State *L);

// gfx functions
static int luisita_luaLoadShader(lua_State *L);
 
#endif


