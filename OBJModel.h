//Headers
#include <iostream>
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>

#ifdef _WIN32
    #include <Windows.h>
    
        //GLEW
    #include "C:/glew-2.1.0/include/GL/glew.h"
        //OpenGL
    #include <GL/gl.h>
#else
        //For XWindows API
    #include <X11/Xlib.h>       //For Xlib API
    #include <X11/Xutil.h>      //For Xlib Utility API
    #include <X11/XKBlib.h>     //For Xlib Keyboard API
    #include <X11/keysym.h>     //For Xlib Key Symbols
    
        //GLEW
    #include <GL/glew.h>
        //OpenGL
    #include <GL/gl.h>
        //GLX Bridging API
    #include <GL/glx.h>
#endif



//vao
typedef struct Model_Data
{
    GLuint vao;
    GLuint vbo_position;
    GLuint vbo_texture;
    GLuint vbo_normals;
    GLuint vbo_elements;
    GLint  numberOfElements;
    GLint  numberOfVertices;
}Model_Data;

void LoadOBJModel( const char*, Model_Data*, int, int, int);
void UnloadOBJModel( Model_Data*);

