#include "luisita.h"
#include "my_opengl.h"

#include <iostream>
#include <cmath>
#include <vector>

enum
{
    STATUS_NEEDS_LOADING,
    STATUS_LOADING,
    STATUS_LOADED,
    STATUS_FINISHED
};

enum
{
    SHADER_VERTEX = 1,
    SHADER_FRAGMENT
};

// Lua state
static lua_State *L;

// status
int status;
int status_shouldDraw;

int luisita_init()
{
    int i;

    status_shouldDraw = 1;
    status = STATUS_NEEDS_LOADING;

    L = lua_open();
    luaL_openlibs(L);

    lua_register(L, "trace", luisita_luaTrace);

    lua_register(L, "loadShader", luisita_luaLoadShader);

    return 1;
}

int luisita_end()
{
    int i;

    if (L)
    {
        lua_close(L);
    }

    return 1;
}

void luisita_run()
{
    SDL_Event event;
    printf("luisita_run\n");
    while (!luisita_hasFinished())
    {
        luisita_render();
        
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                    break;
                case SDL_KEYUP:
                    // If escape is pressed, return (and thus, quit)
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        status = STATUS_FINISHED;
                    break;
                case SDL_QUIT:
                    status = STATUS_FINISHED;
            }
        }
    }
}

int luisita_loadScript(const char *scriptFile)
{
    int res;

    res = luaL_loadfile(L, scriptFile);

    if (res == 0)
    {
        res = lua_pcall(L, 0, LUA_MULTRET, 0);
        luisita_reportErrors(L, res);
    }
    else
    {
        luisita_reportErrors(L, res);
        return 0;
    }

    return 1;
}

int luisita_hasFinished()
{
    if (status == STATUS_FINISHED)
    {
        return 1;
    } else return 0;
}

void luisita_render()
{
    // does the draw function exist in lua space? if so, call it with current time
    lua_getglobal(L, "draw");
    if (lua_isfunction(L, 1))
    {
        lua_pushnumber(L, SDL_GetTicks());
        lua_call(L, 1, 0);
    }
    else
    {
        lua_pop(L, 1);
    }

    SDL_GL_SwapBuffers();
    
}

void luisita_reportErrors(lua_State *L, int st)
{
    if (st != 0)
    {
        std::cerr << "ERROR -- " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1); // removes error message
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Logging functions
// ``````````````````

int luisita_luaTrace(lua_State *L)
{
    const char *msg = lua_tostring(L, 1);
    std::cerr << "trace: " << msg << std::endl;

    return 0;
}


// GFX functions
// ``````````````

static int luisita_luaLoadShader(lua_State *L)
{
    const char *shaderFileName;
    GLenum glShaderType;
    GLuint shaderId;

    shaderFileName = lua_tostring(L, 1);
    std::cerr << "Loading shader " << shaderFileName << std::endl;

    int shaderType = lua_tointeger(L, 2);
    if(shaderType == SHADER_FRAGMENT)
    {
        std::cerr << "Shader is fragment shader" << std::endl;
        glShaderType = GL_FRAGMENT_SHADER;
    }
    else if(shaderType == SHADER_VERTEX)
    {
        std::cerr << "Shader is vertex shader" << std::endl;
        glShaderType = GL_VERTEX_SHADER;
    }
    else
    {
        std::cerr << "Unknown shader type" << std::endl;
        lua_pushnil(L);
        return -1;
    }

    shaderId = glCreateShader(glShaderType);
    
    /*
     void glShaderSource(GLuint shader, int numOfStrings, const char **strings, int *lenOfStrings);

Parameters:

    shader - the handler to the shader.
    numOfStrings - the number of strings in the array.
    strings - the array of strings.
    lenOfStrings - an array with the length of each string, or NULL, meaning that the strings are NULL terminated. 
    
     * 
     void glCompileShader(GLuint shader); 
     shader - the handler to the shader.
     *
     * GLuint glCreateProgram(void);
     *
     * void glAttachShader(GLuint program, GLuint shader);
     *
    program - the handler to the program.
    shader - the handler to the shader you want to attach.
     *
     *
     * void glLinkProgram(GLuint program);
     *
     * void glUseProgram(GLuint prog); 
     * */


    return 0;
}