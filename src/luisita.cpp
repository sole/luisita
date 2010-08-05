#include "luisita.h"
#include "my_opengl.h"

#include <iostream>
#include <cmath>
#include <vector>

#include "bass/bass.h"
#include "SOIL/SOIL.h"

// Miscellaneous constants
enum
{
	COLOR_FLAT,
	COLOR_SMOOTH
};

enum
{
	STATUS_NEEDS_LOADING,
	STATUS_LOADING,
	STATUS_LOADED,
	STATUS_FINISHED
};

enum {
	SHAPE_POINTS,
	SHAPE_LINES,
	SHAPE_LINE_STRIP,
	SHAPE_TRIANGLES,
	SHAPE_TRIANGLE_STRIP,
	SHAPE_QUADS,
	SHAPE_QUAD_STRIP,
	SHAPE_POLYGON
};

enum
{
	BLENDING_ALPHABLEND,
	BLENDING_ADDITIVE,
	BLENDING_SUBSTRACTIVE,
	BLENDING_INVERT, //?
	BLENDING_INVERTDEST,//?
	BLENDING_MASK,//?
	BLENDING_MULTIPLY,
	BLENDING_INVMULTIPLY,//?
	BLENDING_COLORMULTIPLY
};

enum
{
	FOG_NONE,
	FOG_LINEAR,
	FOG_EXP,
	FOG_EXP2
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

// Screen
int screen_width = 640, screen_height = 480; // defaults, just in case...
float				screen_ortho_near = 0.0f;
float				screen_ortho_far = 200.0f;

// camera, perspective
float camera_fov = 60.0f,
	camera_aspect = 1.3f,
	camera_zNear = 1.0f,
	camera_zFar = 100.0f;

// colors
GLfloat				screen_background[4];
GLfloat				fill_color[4];
GLfloat				stroke_color[4];
int					use_stroke;
int					use_fill;

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
	lua_register(L, "width", luisita_luaWidth);
	lua_register(L, "height", luisita_luaHeight);

    lua_register(L, "loadShader", luisita_luaLoadShader);
    
    lua_register(L, "line", luisita_luaLine);
	lua_register(L, "triangle", luisita_luaTriangle);
	lua_register(L, "stipple", luisita_luaStipple);
	lua_register(L, "noStipple", luisita_luaNoStipple);
	
	
	lua_register(L, "background", luisita_luaBackground);
	lua_register(L, "fill", luisita_luaFill);
	lua_register(L, "noFill", luisita_luaNoFill);
	lua_register(L, "stroke", luisita_luaStroke);
	lua_register(L, "noStroke", luisita_luaNoStroke);
	lua_register(L, "color", luisita_luaColor);
	lua_register(L, "colorMode", luisita_luaColorMode);
	
	lua_register(L, "strokeWeight", luisita_luaStrokeWeight);
	lua_register(L, "pointSize", luisita_luaPointSize);
	
	lua_register(L, "beginShape", luisita_luaBeginShape);
	lua_register(L, "endShape", luisita_luaEndShape);
	lua_register(L, "vertex", luisita_luaVertex);
	lua_register(L, "normal", luisita_luaNormal);
	

	lua_register(L, "blending", luisita_luaBlending);
	lua_register(L, "noBlending", luisita_luaNoBlending);
	
	lua_register(L, "loadImage", luisita_luaLoadImage);
	lua_register(L, "image", luisita_luaImage);
	lua_register(L, "useTexture", luisita_luaUseTexture);
	lua_register(L, "noTexture", luisita_luaNoTexture);
	
	lua_register(L, "ortho", luisita_luaOrtho);
	lua_register(L, "restoreProjection", luisita_luaRestoreProjection);
	lua_register(L, "camera", luisita_luaCamera);
	lua_register(L, "perspective", luisita_luaPerspective);	
	
	lua_register(L, "depthTest", luisita_luaDepthTest);
	lua_register(L, "noDepthTest", luisita_luaNoDepthTest);
	
	lua_register(L, "noLights", luisita_luaNoLights);
	lua_register(L, "lightEnabled", luisita_luaLightEnabled);
	lua_register(L, "lightPosition", luisita_luaLightPosition);
	lua_register(L, "lightAmbient", luisita_luaLightAmbient);
	lua_register(L, "lightDiffuse", luisita_luaLightDiffuse);
	lua_register(L, "lightSpotDirection", luisita_luaLightSpotDirection);
	
	lua_register(L, "fogMode", luisita_luaFogMode);
	lua_register(L, "fogDensity", luisita_luaFogDensity);
	lua_register(L, "fogStartEnd", luisita_luaFogStartEnd);
	lua_register(L, "fogColor", luisita_luaFogColor);

	
    lua_register(L, "loadMusicStream", luisita_luaLoadMusicStream);
	lua_register(L, "playMusicStream", luisita_luaPlayMusicStream);
	lua_register(L, "isMusicStreamFinished", luisita_luaIsMusicStreamFinished);
	lua_register(L, "getMusicStreamTime", luisita_luaGetMusicStreamTime);

	screenBackground[0] = 0.3f;
	screenBackground[1] = 0.3f;
	screenBackground[2] = 0.3f;
	screenBackground[3] = 1.0f;
	
	// Setup views
	/*glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	luisita_gfxSetOrtho();*/

    return 1;
}

int luisita_setWindowDimensions(int width, int height)
{
	screen_width = width;
	screen_height = height;
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
	sleep(1);
	status = STATUS_LOADED;
	return 0;
}

int luisita_luaQuit(lua_State *L)
{
	status = STATUS_FINISHED;
	return 0;
}

// Return current window width in pixels
int luisita_luaWidth(lua_State *L)
{
	lua_pushinteger(L, screen_width);
	return 1;
}

// Return current window height in pixels
int luisita_luaHeight(lua_State *L)
{
	lua_pushinteger(L, screen_height);
	return 1;
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



int luisita_luaLine(lua_State *L)
{
	int i = 0;
	float x, y, z;
	int num_elem = lua_gettop(L);
	
	if(num_elem == 6)
	{
		glBegin(GL_LINES);
		while(i < 6)
		{
			x = lua_tonumber(L, i+1); i++;
			y = lua_tonumber(L, i+1); i++;
			z = lua_tonumber(L, i+1); i++;
			glVertex3f(x, y, z);
		}
		glEnd();
	}
	return 0;	 
}

int luisita_luaTriangle(lua_State *L)
{
	int i = 0;
	double values[9];
	double x, y, z;
	int num_elem = lua_gettop(L);
	
	if(num_elem == 9)
	{
		// first get the values
		for(i = 0; i < 9; i++)
		{
			values[i] = lua_tonumber(L, i+1);
		}
		
		if(use_fill)
		{
			glBegin(GL_TRIANGLES);
			i = 0;
			glColor4f(fill_color[0], fill_color[1], fill_color[2], fill_color[3]);
			while(i < 9)
			{
				x = values[i]; i++;
				y = values[i]; i++;
				z = values[i]; i++;
				glVertex3f(x, y, z);
			}
			glEnd();
		}
		
		if(use_stroke)
		{
			glColor4f(stroke_color[0], stroke_color[1], stroke_color[2], stroke_color[3]);
			glBegin(GL_LINE_STRIP);
			i = 0;
			while(i < 9)
			{
				x = values[i]; i++;
				y = values[i]; i++;
				z = values[i]; i++;
				glVertex3f(x, y, z);
			}
			glVertex3f(values[0], values[1], values[2]);
			glEnd();
		}
		
	}
	return 0;
}

int luisita_luaStipple(lua_State *L)
{
	unsigned short pattern = lua_tointeger(L, 1);
	int factor = 1;
	
	if(lua_gettop(L) > 1)
	{
		factor = pattern;
		pattern = lua_tointeger(L, 2);
	}
	
	glEnable(GL_LINE_STIPPLE);
	glLineStipple(factor, pattern);
	
	return 0;
}

int luisita_luaNoStipple(lua_State *L)
{
	glDisable(GL_LINE_STIPPLE);
	
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

int luisita_luaFill(lua_State *L)
{
	int i;
	int num_elem = lua_gettop(L);
	
	if(num_elem == 3 || num_elem == 4)
	{
		for(i = 0; i < 4; i++)
		{
			fill_color[i] = lua_tonumber(L, i+1);
		}
		use_fill = 1;
	}
	return 0;	 
}

int luisita_luaNoFill(lua_State *L)
{
	use_fill = 0;
	return 0;
}

int luisita_luaStroke(lua_State *L)
{
	int i;
	int num_elem = lua_gettop(L);
	
	if(num_elem == 3 || num_elem == 4)
	{
		for(i = 0; i < 4; i++)
		{
			stroke_color[i] = lua_tonumber(L, i+1);
		}
		use_stroke = 1;
		glColor4f(stroke_color[0], stroke_color[1], stroke_color[2], stroke_color[3]);
	}
	return 0;	 
	
}

int luisita_luaNoStroke(lua_State *L)
{
	use_stroke = 0;
	return 0;
}

int luisita_luaColor(lua_State *L)
{
	if(lua_gettop(L) == 4)
	{
		glColor4f(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	return 0;	 
	
}

int luisita_luaColorMode(lua_State *L)
{
	unsigned int mode = lua_tointeger(L, 1);
	
	if(mode == COLOR_FLAT)
	{
		glShadeModel(GL_FLAT);
	}
	else
	{
		glShadeModel(GL_SMOOTH);
	}
	return 0;	 
	
}

// Attributes
//------------------------------------------------------------------------------

int luisita_luaStrokeWeight(lua_State *L)
{
	float weight = lua_tonumber(L, 1);
	glLineWidth(weight);
	return 0;
}

int luisita_luaPointSize(lua_State *L)
{
	float size = lua_tonumber(L, 1);
	glPointSize(size);
	return 0;
}

// Vertex
//------------------------------------------------------------------------------

int luisita_luaBeginShape(lua_State *L)
{
	int shapeType = lua_tointeger(L, 1);
	int t;
	
	switch(shapeType)
	{
		case SHAPE_POINTS: t = GL_POINTS; break;
		case SHAPE_LINES: t = GL_LINES; break;
		case SHAPE_LINE_STRIP: t = GL_LINE_STRIP; break;
		case SHAPE_TRIANGLES: t = GL_TRIANGLES; break;
		case SHAPE_TRIANGLE_STRIP: t = GL_TRIANGLE_STRIP; break;
		case SHAPE_QUADS: t = GL_QUADS; break;
		case SHAPE_QUAD_STRIP: t = GL_QUAD_STRIP; break;
		default:
		case SHAPE_POLYGON: t = GL_POLYGON; break;
	}
	
	glBegin(t);
	
	return 0;
}

int luisita_luaEndShape(lua_State *L)
{
	glEnd();
	return 0;
}

int luisita_luaVertex(lua_State *L)
{
	double x, y, z, u, v;
	
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	z = lua_tonumber(L, 3);

	if(lua_gettop(L) == 5)
	{
		u = lua_tonumber(L, 4);
		v = lua_tonumber(L, 5);
		glTexCoord2f(u, v);
	}
	
	glVertex3d(x, y, z);
	
	return 0;
}

int luisita_luaNormal(lua_State *L)
{
	double x, y, z;
	
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	z = lua_tonumber(L, 3);
	
	glNormal3d(x, y, z);
	
	return 0;
}

// Blendings
//------------------------------------------------------------------------------

int luisita_luaBlending(lua_State *L)
{
	glEnable(GL_BLEND);
	int blending = lua_tointeger(L, 1);
	
	if(blending != BLENDING_SUBSTRACTIVE)
	{
		glBlendEquation(GL_FUNC_ADD);
	}
	
	switch(blending)
	{
		case BLENDING_ADDITIVE:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case BLENDING_SUBSTRACTIVE:
			glBlendEquation(GL_FUNC_SUBTRACT);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			break;
		case BLENDING_MULTIPLY:
			glBlendFunc(GL_ONE, GL_ONE);//GL_SRC_ALPHA_SATURATE
			break;
		case BLENDING_COLORMULTIPLY:
			glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_SRC_COLOR);
			break;
		case BLENDING_ALPHABLEND:
		default:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	
	}
	
	return 0;
}

int luisita_luaNoBlending(lua_State *L)
{
	glDisable(GL_BLEND);
	return 0;
}

// images

int luisita_gfxLoadImage(const char* filename)
{
	int textureId = SOIL_load_OGL_texture
	(
	 filename,
	 SOIL_LOAD_AUTO,
	 SOIL_CREATE_NEW_ID,
	 SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
	 );
	
	if(textureId == 0)
	{
		std::cerr << "SOIL loading error: " << std::endl << SOIL_last_result() << std::endl;
	}
	
	return textureId;
}

/*
 Draw a quad with the image , starting at x, y and using either the image size or width and height
 TODO: use SOIL_load_image after loading an image for finding the width and height (lame isn't it!!)
 */
void luisita_gfxShowImage(int textureId, float x, float y, float width, float height)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, textureId);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glScalef(width, height, 1);
	glTranslatef(x, y, 0);
	
	glColor3f(1,1,1);
	glBegin (GL_QUADS);
	glTexCoord2f (0.0, 0.0);
	glVertex3f (0.0, 0.0, 0.0);
	glTexCoord2f (1.0, 0.0);
	glVertex3f (1.0, 0.0, 0.0);
	glTexCoord2f (1.0, 1.0);
	glVertex3f (1.0, 1.0, 0.0);
	glTexCoord2f (0.0, 1.0);
	glVertex3f (0.0, 1.0, 0.0);
	glEnd ();
	
	glPopMatrix();
	
	glDisable(GL_TEXTURE_2D);
}

int luisita_luaLoadImage(lua_State *L)
{
	const char *imageFile = lua_tostring(L, 1);
	int textureId = luisita_gfxLoadImage(imageFile);
	lua_pushinteger(L, textureId);
	
	return 1;
}

int luisita_luaImage(lua_State *L)
{
	if(lua_gettop(L) == 5)
	{
		int textureId = lua_tointeger(L, 1);
		float x = lua_tonumber(L, 2);
		float y = lua_tonumber(L, 3);
		float width = lua_tonumber(L, 4);
		float height = lua_tonumber(L, 5);
		
		luisita_gfxShowImage(textureId, x, y, width, height);
	}
	
	return 0;
}

int luisita_luaUseTexture(lua_State *L)
{
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, lua_tointeger(L, 1));
	
	return 0;
}

int luisita_luaNoTexture(lua_State *L)
{
	glDisable(GL_TEXTURE_2D);
	return 0;
}


// Cameras, views and co
void luisita_gfxSetOrtho(float xres, float yres)
{
	float xhalf, yhalf;
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	//glLoadIdentity();
	
	xhalf = (xres==-1) ? (screen_width)*0.5 : xres*0.5;
	yhalf = (yres==-1) ? (screen_height)*0.5 : yres*0.5;
	glViewport(0, 0, screen_width, screen_height);
	glOrtho(-xhalf, xhalf, -yhalf, yhalf, screen_ortho_near, screen_ortho_far);
}

void luisita_gfxRestoreProjection()
{
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
}

// Cameras
//------------------------------------------------------------------------------

int luisita_luaOrtho(lua_State *L)
{
	if(lua_gettop(L) == 2)
	{
		int width = lua_tonumber(L, 1);
		int height = lua_tonumber(L, 2);
		luisita_gfxSetOrtho(width, height);
	}
	else
	{
		luisita_gfxSetOrtho();
	}
	return 0;
}

int luisita_luaRestoreProjection(lua_State *L)
{
	luisita_gfxRestoreProjection();
	return 0;
}

int luisita_luaCamera(lua_State *L)
{
	float eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ;
	
	if(lua_gettop(L) < 9)
	{
		eyeX = 0;
		eyeY = 0;
		eyeZ = 0;
		
		centerX = 0;
		centerY = 0;
		centerZ = 0;
		
		upX = 0;
		upY = 1;
		upZ = 0;
	}
	else
	{
		eyeX = lua_tonumber(L, 1);
		eyeY = lua_tonumber(L, 2);
		eyeZ = lua_tonumber(L, 3);
		
		centerX = lua_tonumber(L, 4);
		centerY = lua_tonumber(L, 5);
		centerZ = lua_tonumber(L, 6);
		
		upX = lua_tonumber(L, 7);
		upY = lua_tonumber(L, 8);
		upZ = lua_tonumber(L, 9);
	}
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	
	glLoadIdentity();
	gluPerspective(camera_fov, camera_aspect, camera_zNear, camera_zFar);
	gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ);
	
	return 0;
}

int luisita_luaPerspective(lua_State *L)
{
	if(lua_gettop(L) < 4)
	{
		camera_fov = 60;
		camera_aspect = screen_width * 1.0 / screen_height;
		camera_zNear = 1;
		camera_zFar = 100;
	}
	else
	{
		camera_fov = lua_tonumber(L, 1);
		camera_aspect = lua_tonumber(L, 2);
		camera_zNear = lua_tonumber(L, 3);
		camera_zFar = lua_tonumber(L, 4);
	}
	
	return 0;
}

int luisita_luaDepthTest(lua_State *L)
{
	glEnable(GL_DEPTH_TEST);
	return 0;
}

int luisita_luaNoDepthTest(lua_State *L)
{
	glDisable(GL_DEPTH_TEST);
	return 0;
}

// Lights
// -----------------------------------------------------------------------------
int getLightIndex(unsigned int index)
{
	
	switch(index)
	{
		case 1: return GL_LIGHT1;
		case 2: return GL_LIGHT2;
		case 3: return GL_LIGHT3;
		case 4: return GL_LIGHT4;
		case 5: return GL_LIGHT5;
		case 6: return GL_LIGHT6;
		case 7: return GL_LIGHT7;
		
		default:
		case 0: return GL_LIGHT0;
	}
}

int luisita_luaNoLights(lua_State *L)
{
	glDisable(GL_LIGHTING);
	return 0;
}

int luisita_luaLightEnabled(lua_State *L)
{
	unsigned int light_index = getLightIndex(lua_tointeger(L, 1));
	bool status = lua_toboolean(L, 2);
	
	if(status)
	{
		glEnable(light_index);
		// And enable lighting too, just in case
		// This might [should?] be optimised
		glEnable(GL_LIGHTING);
	}		
	else
	{
		glDisable(light_index);
	}
			
	return 0;
}

void setLightParameter4f(lua_State *L, GLenum paramName)
{
	unsigned int index = getLightIndex(lua_tointeger(L, 1));
	
	float lightParams[4] = {
		lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5)
	};
	
	glLightfv(
			  index,
			  paramName,
			  lightParams
			  );
	
}

int luisita_luaLightPosition(lua_State *L)
{
	setLightParameter4f(L, GL_POSITION);
	return 0;
}

int luisita_luaLightAmbient(lua_State *L)
{
	setLightParameter4f(L, GL_AMBIENT);
	return 0;
}

int luisita_luaLightDiffuse(lua_State *L)
{
	setLightParameter4f(L, GL_DIFFUSE);
	return 0;
}

int luisita_luaLightSpotDirection(lua_State *L)
{
	setLightParameter4f(L, GL_SPOT_DIRECTION);
	return 0;
}

// Fog
//------------------------------------------------------------------------------

int luisita_luaFogMode(lua_State *L)
{
	switch(lua_tointeger(L, 1))
	{
		case FOG_NONE:
			glDisable(GL_FOG);
			return 0;
			break;
		case FOG_LINEAR: glFogi(GL_FOG_MODE, GL_LINEAR); break;
		case FOG_EXP: glFogi(GL_FOG_MODE, GL_EXP); break;
		case FOG_EXP2: glFogi(GL_FOG_MODE, GL_EXP2); break;
	}
	
	glEnable(GL_FOG);
	glHint(GL_FOG_HINT, GL_NICEST);
	
	return 0;
}

int luisita_luaFogDensity(lua_State *L)
{
	glFogf(GL_FOG_DENSITY, lua_tonumber(L, 1));
	return 0;
}

int luisita_luaFogStartEnd(lua_State *L)
{
	double start, end;
	
	start = lua_tonumber(L, 1);
	end = lua_tonumber(L, 2);
	
	glFogf(GL_FOG_START, start);
	glFogf(GL_FOG_END, end);
	
	return 0;
}

int luisita_luaFogColor(lua_State *L)
{
	float fogColor[4] = { lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)	};

	glFogfv(GL_FOG_COLOR, fogColor);
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
