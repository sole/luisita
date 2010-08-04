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

// misc functions
static int luisita_luaWindowTitle(lua_State *L);
static int luisita_luaSetupIsDone(lua_State *L);
static int luisita_luaQuit(lua_State *L);

// gfx functions
static int luisita_luaLoadShader(lua_State *L);
static int luisita_luaBackground(lua_State *L);

// audio functions
static int luisita_luaLoadMusicStream(lua_State *L);
static int luisita_luaPlayMusicStream(lua_State *L);
static int luisita_luaIsMusicStreamFinished(lua_State *L);
static int luisita_luaGetMusicStreamTime(lua_State *L);

#endif


