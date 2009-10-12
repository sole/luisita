#include <iostream>
#include <cstdlib>
#include <getopt.h>
#include "luisita.h"
#include "my_opengl.h"

/**
 
"luisita"
 
Start date ~ 10th october 2009
Released yyyymmdd
 
-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 
Copyright 2009 Soledad Penades http://soledadpenades.com

This file is part of luisita

**/

void list_modes()
{
	SDL_Rect** modes;
	int i;

	modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);

	if (modes == (SDL_Rect**)0)
	{
		fprintf(stdout, "No modes available!\n");
		exit(-1);
	}

	if (modes == (SDL_Rect**)-1)
	{
		fprintf(stdout, "All resolutions available.\n");
		exit(0);
	}
	else
	{
		printf("Available Fullscreen Modes\n");
		for (i=0; modes[i]; ++i)
		{
			fprintf(stdout, "%dx%d\n", modes[i]->w, modes[i]->h);
		}

		exit(1);
	}
}

void parse_arguments(int argc, char*argv[], int *width, int *height, int *fullscreen, int *multiSampling, int *multiSamplingBuffers, float *seconds, std::string *scriptName)
{
	int option_index = 0, c;

	// Set some defaults to begin with
	*fullscreen = 0;
	*width = 1024;
	*height = 768;
	*multiSampling = 0;
	*multiSamplingBuffers = 4;
	*seconds = 0;
	*scriptName = "";

	static struct option long_options[] =
	{
		{"fullscreen", no_argument, fullscreen, 1},
		{"width", required_argument, NULL, 'w'},
		{"height", required_argument, NULL, 'h'},
		{"antialias", no_argument, multiSampling, 1},
		{"antialias-samples", required_argument, NULL, 'a'},
		{"start-time", required_argument, NULL, 't'},
		{"script-name", required_argument, NULL, 's'},
		{"list-modes", no_argument, NULL, 'l'},
		{"help", no_argument, NULL, '?'},
		{0, 0, 0, 0}
	};

	while(1)
	{
		c = getopt_long (argc, argv, "w:h:o:r:l:s:t", long_options, &option_index);

		if(c == -1) { break; }

		switch (c)
		{
			case 'w':
				*width = atoi((char*)optarg);
				break;

			case 'h':
				*height = atoi((char*)optarg);
				break;

			case 'a':
				*multiSamplingBuffers = atoi((char*)optarg);
				break;

			case 't':
				*seconds = atof((char*)optarg);
				break;

			case 's':
				scriptName->assign((char*) optarg);
				break;

			case 'l':
				list_modes();
				break;

			case '?':
				printf("Usage:\n"
				       "%s [options]\n\n"
				       "where options is one or more of the following:\n\n"
				       "-w, --width WIDTH: screen width in pixels (default=1024)\n"
				       "-h, --height HEIGHT: screen height in pixels (default=768)\n"
				       "--fullscreen: use fullscreen (default=no)\n"
				       "--antialias: use antialias (default=no)\n"
				       "--antialias-samples: how many antialias samples to use (default=4)\n"
				       "-t, --start-time SECONDS: begin playing at SECONDS time (default=0)\n"
				       "-s, --script-name filename: name of the script to execute (required)\n",
				       "-l, --list-modes: show available accelerated fullscreen size combinations\n"
				       "-?, --help: show this message\n"
				       , argv[0]);
				exit(0);
		}
	}
}

void lastFunction(void)
{
	luisita_end();
	SDL_Quit();
}
 
int main(int argc, char *argv[])
{
    int flags;
    SDL_Surface* screen;
    int width, height, fullscreen, multiSampling, multiSamplingBuffers;
    float startTime;

    std::string scriptName;

    // Initialize SDL's subsystems
    if (SDL_Init(SDL_INIT_VIDEO) < 0 )
    {
            fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
            exit(1);
    }

    // Parse command line parameters
    parse_arguments(argc, argv, &width, &height, &fullscreen, &multiSampling, &multiSamplingBuffers, &startTime, &scriptName);

    if(scriptName.length() == 0)
    {
            fprintf(stderr, "No script name specified, nothing to do?\n");
            exit(1);
    }

    printf("Using width = %d, height = %d, fullscreen = %d\nstart time = %f\nScript name = %s\n", width, height, fullscreen, startTime, scriptName.c_str());

    // Cleanup
    atexit(lastFunction);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    flags = SDL_OPENGL;

    if(fullscreen)
    {
        flags |= SDL_FULLSCREEN;
    }

    if(multiSampling)
    {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, true);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multiSamplingBuffers);
    }
    // vsync, SDL manual says it's on by default but I highly doubt it
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    // Attempt to create the opengl window with 32bit pixels.
    screen = SDL_SetVideoMode(width, height, 32, flags);

    // If we fail, return error.
    if (screen == NULL)
    {
        fprintf(stderr, "Unable to set %dx%d opengl video (fullscreen = %d, antialias samples = %d): %s\n", width, height, fullscreen, multiSamplingBuffers, SDL_GetError());
        list_modes();
        exit(1);
    }

    if(!luisita_init() || !luisita_loadScript(scriptName.c_str()))
    {
        exit(-1);
    }

    GLenum err = glewInit();

    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        // TODO: tell luisita she can't use shaders
    }

    // Main loop
    luisita_run();
 
    return 0;
}


