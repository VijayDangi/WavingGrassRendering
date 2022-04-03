#include "LoadShaders.h"

extern FILE *gpLogFile;


//function declaration
char *getShaderType( GLenum type);

//
//ReadShaderFromFile
//
const char *ReadShaderFromFile(const char *fileName)
{
	//code
	if (fileName == NULL)
		return(NULL);

	FILE *fp = NULL;
#ifdef _WIN32
	fopen_s(&fp, fileName, "r");
#else
	fp = fopen(fileName, "r");
#endif

	if (fp == NULL)
	{
		fprintf(gpLogFile, "Error While Opening File '%s'\n", fileName);
		return(NULL);
	}

	fseek(fp, 0, SEEK_END);
	int length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *source = (char *) calloc(1, length + 1);

	char ch;
	int i = 0;
	while( (ch = fgetc(fp)) != EOF)
		source[i++] = ch;

	*(source + i) = '\0';

	fclose(fp);
	fp = NULL;

	return((const char *)source);
}


//
//compileShaderFromSource
//
GLuint compileShaderFromSource( const char *shaderSourceCode, GLenum shaderType)
{
    GLint shaderCompiledStatus, infoLogLength;
    char *szInfoLog = NULL;

	if( shaderSourceCode == NULL)
		return(-1);

    GLuint shaderID = glCreateShader( shaderType);
    
    glShaderSource( shaderID, 1, (const GLchar**) &shaderSourceCode, NULL);
    glCompileShader( shaderID);
    
    glGetShaderiv( shaderID, GL_COMPILE_STATUS, &shaderCompiledStatus);
    if(shaderCompiledStatus == GL_FALSE)    //Compilation Failed
    {
        //get shader log length
        glGetShaderiv( shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        if(infoLogLength > 0)
        {
            //allocate memory for log
            szInfoLog = (char *) malloc( sizeof(char) * infoLogLength);
            if(szInfoLog != NULL)
            {
                glGetShaderInfoLog( shaderID, infoLogLength, NULL, szInfoLog);
                fprintf( gpLogFile, "ERROR: %s Compilation Failed : %s \n", getShaderType( shaderType), szInfoLog);
                
                //free memory
                free(szInfoLog);
				szInfoLog = NULL;
				glDeleteShader( shaderID);
				return(-1);
            }
        }

        return( -1);
    }

    return( shaderID);
}

//
//compileShaderFromFile
//
GLuint compileShaderFromFile( const char *fileName, GLenum shaderType)
{
    GLint shaderCompiledStatus, infoLogLength;
    char *szInfoLog = NULL;

	const char *shaderSource = ReadShaderFromFile( fileName);
    if( shaderSource == NULL)
    {
        fprintf( gpLogFile, "Shader Loading from File '%s' Failed\n", fileName);
        return(-1);
    }

    GLuint shaderID = compileShaderFromSource( shaderSource, shaderType);
    free( (void *)shaderSource);
	shaderSource = NULL;

    return( shaderID);
}

//
//getShaderType()
//
char *getShaderType( GLenum type)
{
	//code
	switch( type)
	{
		case GL_VERTEX_SHADER:
			return("GL_VERTEX_SHADER");
		break;

		case GL_FRAGMENT_SHADER:
			return("GL_FRAGMENT_SHADER");
		break;

		case GL_TESS_CONTROL_SHADER:
			return("GL_TESS_CONTROL_SHADER");
		break;

		case GL_TESS_EVALUATION_SHADER:
			return("GL_TESS_EVALUATION_SHADER");
		break;

		case GL_GEOMETRY_SHADER:
			return("GL_GEOMETRY_SHADER");
		break;
	}

	return("UNKNOWN SHADER TYPE");
}


//
//CreateProgram
//
GLuint CreateProgram(SHADERS_INFO *shaderInfo, int shaderCount, BIND_ATTRIBUTES_INFO *attribInfo, int attribCount, int lineNo)
{
	//variables
	const char *source;
	GLuint programID;

	GLint infoLogLength = 0;
	GLint shaderCompiledStatus, programLinkStatus;
	char *szInfoLog = NULL;

	SHADERS_INFO *shader;

	//code
	programID = glCreateProgram();

	for(int i = 0; i < shaderCount; i++)
	{
		shader = shaderInfo + i;

		if (shader->shaderLoadAs == CREATE_PROGRAM_SHADER_LOAD_FROM_FILE)
		{
			source = ReadShaderFromFile(shader->shaderFileName);
			if (source == NULL)
			{
				if (shader->shaderID == 0)
				{
					fprintf(gpLogFile, "(%d): Error while creating shader %s\n", lineNo, getShaderType(shader->shaderType));
					for( int j = 0; j < i; j++)
					{
						glDetachShader(programID, (shaderInfo + j)->shaderID);
						glDeleteShader(( shaderInfo + j)->shaderID);
						( shaderInfo + j)->shaderID = 0;
					}

					glDeleteProgram(programID);
					return(0);
				}
			}
		}
		else
			source = shader->shaderSource;



		shader->shaderID = glCreateShader(shader->shaderType);
		if (shader->shaderID == 0)
		{
			fprintf(gpLogFile, "(%d): Error while creating shader %s\n", lineNo, getShaderType(shader->shaderType));
			for( int j = 0; j < i; j++)
			{
				glDetachShader(programID, (shaderInfo + j)->shaderID);
				glDeleteShader(( shaderInfo + j)->shaderID);
				( shaderInfo + j)->shaderID = 0;
			}

			if (shader->shaderLoadAs == CREATE_PROGRAM_SHADER_LOAD_FROM_FILE)
			{
				free((void*)source);
				source = NULL;
			}

			glDeleteProgram(programID);
			return(0);
		}

		//feed source code to shader object
		glShaderSource( shader->shaderID, 1, (const GLchar**) &source, NULL);

		//Compile Source code
		glCompileShader( shader->shaderID);

		//Compilation status
		glGetShaderiv( shader->shaderID, GL_COMPILE_STATUS, &shaderCompiledStatus);
		if( shaderCompiledStatus == GL_FALSE)   //Compilation Failed
		{
			//get shader compile log length
			glGetShaderiv( shader->shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
			if(infoLogLength > 0)
			{
				//allocate memory for log
				szInfoLog = (char *) malloc( sizeof(char) * infoLogLength);
				if(szInfoLog != NULL)
				{
					//get log
					glGetShaderInfoLog( shader->shaderID, infoLogLength, NULL, szInfoLog);
					fprintf(gpLogFile, "Error (%d): %s COMPILE LOG:\n%s \n", lineNo, getShaderType(shader->shaderType), szInfoLog);

					//free memory
					free(szInfoLog);

					for( int j = 0; j < i; j++)
					{
						glDetachShader(programID, (shaderInfo + j)->shaderID);
						glDeleteShader(( shaderInfo + j)->shaderID);
						( shaderInfo + j)->shaderID = 0;
					}

					if (shader->shaderLoadAs == CREATE_PROGRAM_SHADER_LOAD_FROM_FILE)
					{
						free((void*)source);
						source = NULL;
					}

					glDeleteProgram(programID);
					return(0);
				}
			}
		}

		if (shader->shaderLoadAs == CREATE_PROGRAM_SHADER_LOAD_FROM_FILE)
		{
			free((void*)source);
			source = NULL;
		}

		glAttachShader(programID, shader->shaderID);
	}

	//bind attributes
	for (int i = 0; i < attribCount; i++)
		glBindAttribLocation(programID, (attribInfo + i)->index, (attribInfo + i)->attribute);


	//Link Program
	glLinkProgram(programID);

	//linking status
	glGetProgramiv(programID, GL_LINK_STATUS, &programLinkStatus);
	if (programLinkStatus == GL_FALSE)   //Linking Failed
	{
		//get link log length
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			//allocate memory for log
			szInfoLog = (char*)malloc(infoLogLength);
			if (szInfoLog != NULL)
			{
				glGetProgramInfoLog(programID, infoLogLength, NULL, szInfoLog);
				fprintf(gpLogFile, "(%d): /*** SHADER LINK LOG ***/ %s \n", lineNo, szInfoLog);

				//free memory
				free(szInfoLog);

				for( int j = 0; j < shaderCount; j++)
				{
					glDetachShader(programID, (shaderInfo + j)->shaderID);
					glDeleteShader(( shaderInfo + j)->shaderID);
					( shaderInfo + j)->shaderID = 0;
				}

				glDeleteProgram(programID);
				return(0);
			}
		}
	}

	return(programID);
}



//
//DeleteProgram()
//
void DeleteProgram(GLuint program)
{
	//varriables
	GLsizei shaderCount;
	GLsizei actualShaderCount;

	//code
	glUseProgram(program);

	glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaderCount);
	GLuint *pShader = (GLuint *)malloc(shaderCount * sizeof(GLuint));

	glGetAttachedShaders(program, shaderCount, &actualShaderCount, pShader);
	
	for (int i = 0; i < shaderCount; i++)
	{
		glDetachShader(program, pShader[i]);
		glDeleteShader(pShader[i]);
	}

	glDeleteProgram(program);
	program = 0;

	glUseProgram(0);

	free(pShader);
	pShader = 0;
}

