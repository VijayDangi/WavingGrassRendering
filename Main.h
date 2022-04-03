#ifndef __MAIN_H__
#define __MAIN_H__

#include <Windows.h>
#include <stdio.h>

    //OpenGL and GLEW library
#include <GL/glew.h>
#include <gl/GL.h>

#include "vmath.h"
#include "MyMath.h"

#include "LoadShaders.h"

#include "CustomStruct.h"

//============================== Common Variables
extern HWND ghwnd;
extern FILE *gpLogFile;

//========================== function prototypes
    //TextureLoading.cpp
GLuint LoadTexture( char *fileName);

#endif


