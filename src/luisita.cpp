#include "luisita.h"
#include "my_opengl.h"

#include <iostream>
#include <cmath>
#include <vector>

#include "bass/bass.h"

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

// Music/Audio
std::vector<HSTREAM> musicStreams;

// GFX
GLfloat screenBackground[4];

// ---------------------------------


int luisita_init()
{
    int i;
	
    status_shouldDraw = 1;
    status = STATUS_NEEDS_LOADING;
    
    if (!BASS_Init(-1, 44100,0,0,NULL))
	{
		printf("Music device couldn't be initialized (error code = %d)\n", BASS_ErrorGetCode());
		return(0);
	}

    L = lua_open();
    luaL_openlibs(L);

    lua_register(L, "trace", luisita_luaTrace);
    
    lua_register(L, "windowTitle", luisita_luaWindowTitle);
    lua_register(L, "setupIsDone", luisita_luaSetupIsDone);
	lua_register(L, "quit", luisita_luaQuit);

    lua_register(L, "loadShader", luisita_luaLoadShader);
    lua_register(L, "background", luisita_luaBackground);
    
    lua_register(L, "loadMusicStream", luisita_luaLoadMusicStream);
	lua_register(L, "playMusicStream", luisita_luaPlayMusicStream);
	lua_register(L, "isMusicStreamFinished", luisita_luaIsMusicStreamFinished);
	lua_register(L, "getMusicStreamTime", luisita_luaGetMusicStreamTime);

	screenBackground[0] = 0.3f;
	screenBackground[1] = 0.3f;
	screenBackground[2] = 0.3f;
	screenBackground[3] = 1.0f;

    return 1;
}

int luisita_end()
{
    int i;

	for(i = 0; i < musicStreams.size(); i++)
	{
		BASS_StreamFree(musicStreams.at(i));
	}

	BASS_Free();

    if (L)
    {
        lua_close(L);
    }

    return 1;
}

void luisita_run()
{
    SDL_Event event;
    
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
	if(status == STATUS_NEEDS_LOADING)
	{
		// does the setup function exist in lua space?
		lua_getglobal(L, "setup");
		if(lua_isfunction(L, 1))
		{
			lua_call(L, 0, 0);
			// setup should call swap_buffers via progress bar and stay there until it finishes :)
		}
		else
		{
			// end of story!
			status = STATUS_LOADED;
		}
	}
	
	if(status == STATUS_LOADED)
	{
		//glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
		//glClear(GL_COLOR_BUFFER_BIT);
		
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

// Misc functions
// ``````````````

int luisita_luaWindowTitle(lua_State *L)
{
	const char *text = lua_tostring(L, 1);
	SDL_WM_SetCaption(text, NULL);
	
	return 0;
}

int luisita_luaSetupIsDone(lua_State *L)
{
	// When setup is done, give some time for the system to 'relax'
	sleep(3);
	status = STATUS_LOADED;
	return 0;
}

int luisita_luaQuit(lua_State *L)
{
	status = STATUS_FINISHED;
	return 0;
}


// GFX functions
// ``````````````

// NOTE this obviously is not finished yet!!! someday I'll have time to look into shaders...
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

int luisita_luaBackground(lua_State *L)
{
	int i;
	for(i = 0; i < 4; i++)
	{
		screenBackground[i] = lua_tonumber(L, i+1);
	}
	glClearColor(screenBackground[0], screenBackground[1], screenBackground[2], screenBackground[3]);
	glClear(GL_COLOR_BUFFER_BIT);
	
	return 0;	 
}

// AUDIO functions
// ```````````````

int luisita_luaLoadMusicStream(lua_State *L)
{
	const char *path = lua_tostring(L, 1);

	HSTREAM stream = BASS_StreamCreateFile(FALSE, path, 0, 0, BASS_STREAM_PRESCAN);

	if(stream)
	{
		musicStreams.push_back(stream);
		lua_pushinteger(L, musicStreams.size() - 1);
	}
	else
	{
		printf("Can't load stream file %s (error code = %d)\n", path, BASS_ErrorGetCode());		
		lua_pushnil(L);
	}

	return 1;
}

int luisita_luaPlayMusicStream(lua_State *L)
{
	float positionMillis = 0;
	int index = lua_tointeger(L, 1);

	if(lua_gettop(L) == 2)
	{
		positionMillis = lua_tonumber(L, 2);
	}

	if(index + 1 <= musicStreams.size())
	{
		BASS_ChannelPlay(musicStreams.at(index), FALSE);
		if(positionMillis > 0)
		{
			BASS_ChannelSetPosition(musicStreams.at(index), BASS_ChannelSeconds2Bytes(musicStreams.at(index), positionMillis / 1000), BASS_POS_BYTE);
		}
	}
	return 0;
}

int luisita_luaIsMusicStreamFinished(lua_State *L)
{
	int stream = musicStreams.at(lua_tointeger(L, 1));
	QWORD pos = BASS_ChannelGetPosition(stream, BASS_POS_BYTE);
	QWORD len = BASS_ChannelGetLength(stream, BASS_POS_BYTE);

	lua_pushboolean(L, pos >= len);

	return 1;
}

int luisita_luaGetMusicStreamTime(lua_State *L)
{
	// This value is the current position of the stream in SECONDS
	int channel = musicStreams.at(lua_tointeger(L, 1));
	lua_pushnumber(L, BASS_ChannelBytes2Seconds(channel, BASS_ChannelGetPosition(channel, BASS_POS_BYTE)) );
	return 1;
}
