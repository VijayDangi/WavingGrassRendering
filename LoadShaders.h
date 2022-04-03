#ifndef __LOAD_SHADERS_H__
#define __LOAD_SHADERS_H__

#include <Windows.h>
#include <stdio.h>
    //Graphic Library Extension Wrangler
#include <GL/glew.h>    //Must be included before "gl.h"

#include <gl/gl.h>

typedef struct BIND_ATTRIBUTES_INFO
{
	const char *attribute;
	GLuint index;

} BIND_ATTRIBUTES_INFO;


#define CREATE_PROGRAM_SHADER_LOAD_FROM_FILE	0x0
#define CREATE_PROGRAM_SHADER_LOAD_FROM_STRING  0x1
typedef struct SHADERS_INFO
{
	GLenum shaderType;
	GLenum shaderLoadAs;
	union
	{
		const char *shaderSource;
		const char *shaderFileName;
	};

	GLuint shaderID;

}SHADERS_INFO;

//function declaration
const char *ReadShaderFromFile(const char *fileName);
GLuint compileShaderFromFile( const char *fileName, GLenum shaderType);
GLuint compileShaderFromSource( const char *shaderSourceCode, GLenum shaderType);

GLuint CreateProgram(SHADERS_INFO * shaderInfo, int shaderCount, BIND_ATTRIBUTES_INFO * attribInfo, int attribCount, int lineNo);
void DeleteProgram(GLuint program);

#endif
