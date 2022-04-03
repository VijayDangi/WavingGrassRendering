#include <stdio.h>
#include <vector>

    //Graphic Library Extension Wrangler
#include <gl/glew.h>    //Must be included before "gl.h"
    //OpenGL API
#include <gl/gl.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "FreeType2DText.h"
#include "CustomStruct.h"
#include "LoadShaders.h"
#include "vmath.h"

//Library Link
#pragma comment( lib, "freetype.lib")

extern HWND ghwnd;
extern FILE *gpLogFile;

//program
static GLuint text2D_program;

static GLint uniform_text_ScreenSize;
static GLint uniform_text_TextSampler;
static GLint uniform_text_Color;

//vertex buffer
static GLuint vao_text;
static GLuint vbo_textVertexBuffer;
static GLuint vbo_textUVBuffer;

struct CHARACTER
{
    GLuint textureID;       //ID handle of the glyph texture
    
    struct{
        GLuint x;
        GLuint y;
    }size;                  //size of glyph

    struct{
        GLint x;
        GLint y;
    }bearing;               //offset from baseline to left/top of glyph

    GLint advance;         //horizontal offset to advance to next glyph
};

struct FreeTypeFont
{
    std::vector<struct CHARACTER> characterMap;

    float textColor[4];
    float cursorPos[2];
    float fontHeight;
    float fontScaleW;
    float fontScaleH;
    float newLineXPos;

    FreeTypeFont()
    {
        cursorPos[0] = 0.0f;
        cursorPos[1] = 0.0f;

        fontHeight = 0.0f;
        fontScaleW = 1.0f;
        fontScaleH = 1.0f;
        newLineXPos = 0.0f;

        textColor[0] = 1.0f;
        textColor[1] = 1.0f;
        textColor[2] = 1.0f;
        textColor[3] = 1.0f;
    }

    void Delete()
    {
        for( int i = 0; i < characterMap.size(); i++)
        {
            DELETE_TEXTURE(characterMap[i].textureID);
        }

        characterMap.clear();
    }
};

//
//FontRendringInitialize_FreeType()
//
void FontRendringInitialize_FreeType( void)
{
    //////Text2D Program
    SHADERS_INFO shaderInfo[2];

	shaderInfo[0].shaderType = GL_VERTEX_SHADER;
	shaderInfo[0].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[0].shaderFileName = "shaders/font_2d/freeType/vertex_shader.glsl";
	shaderInfo[0].shaderID = 0;

	shaderInfo[1].shaderType = GL_FRAGMENT_SHADER;
	shaderInfo[1].shaderLoadAs = CREATE_PROGRAM_SHADER_LOAD_FROM_FILE;
	shaderInfo[1].shaderFileName = "shaders/font_2d/freeType/fragment_shader.glsl";
	shaderInfo[1].shaderID = 0;

	BIND_ATTRIBUTES_INFO text_bindAttributes[] =
	{
		{"vPosition", VJD_ATTRIBUTE_POSITION},
        {"vTexCoord", VJD_ATTRIBUTE_TEXTCOORD}
	};

	text2D_program = CreateProgram(shaderInfo, _ARRAYSIZE(shaderInfo), text_bindAttributes, _ARRAYSIZE(text_bindAttributes), __LINE__);
	if (text2D_program == 0)
		DestroyWindow(ghwnd);

    fprintf(gpLogFile, "------------- TEXT2D PROGRAM UNIFORM ----------------------\n");
    fprintf( gpLogFile, "%d\n", uniform_text_ScreenSize = glGetUniformLocation(text2D_program, "ScreenSize"));
    fprintf( gpLogFile, "%d\n", uniform_text_TextSampler = glGetUniformLocation(text2D_program, "TextSampler"));
    fprintf( gpLogFile, "%d\n", uniform_text_Color = glGetUniformLocation(text2D_program, "Color"));
    fprintf(gpLogFile, "--------------------------------------------------------\n");


    //buffer
        //text rendering buffer
    glGenVertexArrays(1, &vao_text);
    glBindVertexArray( vao_text);
        glGenBuffers( 1, &vbo_textVertexBuffer);
        glBindBuffer( GL_ARRAY_BUFFER, vbo_textVertexBuffer);
        glBufferData( GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
        glVertexAttribPointer( VJD_ATTRIBUTE_POSITION, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray( VJD_ATTRIBUTE_POSITION);
        glBindBuffer( GL_ARRAY_BUFFER, 0);

        glGenBuffers( 1, &vbo_textUVBuffer);
        glBindBuffer( GL_ARRAY_BUFFER, vbo_textUVBuffer);
        glBufferData( GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), NULL, GL_STATIC_DRAW);
        glVertexAttribPointer( VJD_ATTRIBUTE_TEXTCOORD, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray( VJD_ATTRIBUTE_TEXTCOORD);
        glBindBuffer( GL_ARRAY_BUFFER, 0);
    glBindVertexArray( 0);

}

//
//FontCreate()
//
FreeTypeFont* FontCreate( const char *fileName, int fontSize)
{
    ////////////////////////////////////////////////
    FT_Library ft;
    if( FT_Init_FreeType( &ft))
    {
        fprintf( gpLogFile, "%s (%d) Error: Could not initialize FreeType Library\n", __FILE__, __LINE__);
        return (NULL);
    }

    FT_Face face;
    if(FT_New_Face( ft, fileName, 0, &face))
    {
        fprintf( gpLogFile, "%s (%d) Error: Failed to Load Font \"%s\"\n", __FILE__, __LINE__, fileName);
        FT_Done_FreeType( ft);
        return (NULL);
    }

    FreeTypeFont *freeFont = (FreeTypeFont *) calloc( sizeof(FreeTypeFont), 1);
    if( freeFont == NULL)
    {
        fprintf( gpLogFile, "%s (%d) Error: Memory allocation failed\n", __FILE__, __LINE__);

        FT_Done_Face( face);
        FT_Done_FreeType( ft);

        return(NULL);
    }

    FT_Set_Pixel_Sizes( face, 0, fontSize);   //setting width to 0, lets the face dynamically calculate the width based on the given height.
    freeFont->fontHeight = fontSize;

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

    for( unsigned char c = 0; c < 128; c++)
    {
        if( FT_Load_Char( face, c, FT_LOAD_RENDER))
        {
            fprintf( gpLogFile, "Error: Failed to load Glyph\n");
            continue;
        }

        //generate texture
        GLuint texture = 0;
        glGenTextures( 1, &texture);
        glBindTexture( GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        //fprintf( gpLogFile, "%d: %d, %d\n", c, face->glyph->bitmap.width, face->glyph->bitmap.rows);

        //set texture options
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        struct CHARACTER character = {
            texture,
            { face->glyph->bitmap.width, face->glyph->bitmap.rows },
            { face->glyph->bitmap_left, face->glyph->bitmap_top },
            (GLint)face->glyph->advance.x
        };

            //push sequencially so access by direct index i.e. charcter
        freeFont->characterMap.push_back(character);

        glBindTexture( GL_TEXTURE_2D, 0);
    }

    //destroy freetype
    FT_Done_Face( face);
    FT_Done_FreeType( ft);

    return (freeFont);
}

//
//FontGetSize()
//
float FontGetSize( FreeTypeFont *freeFont)
{
    return ( freeFont->fontHeight);
}

//
//FontSetColor()
//
void FontSetColor_FreeType(FreeTypeFont *freeFont, float r, float g, float b, float a)
{
    freeFont->textColor[0] = r;
    freeFont->textColor[1] = g;
    freeFont->textColor[2] = b;
    freeFont->textColor[3] = a;
}

//
//FontMoveNewLine()
//
void FontNewLine_FreeType( FreeTypeFont *freeFont)
{
    //code
    freeFont->cursorPos[0] = freeFont->newLineXPos;
    freeFont->cursorPos[1] = freeFont->cursorPos[1] - freeFont->fontHeight * 0.5f * freeFont->fontScaleH;
}

//
//FontSetNewLineXPosition()
//
void FontSetNewLineXPosition_FreeType( FreeTypeFont *freeFont, float x)
{
    //code
    freeFont->newLineXPos = x;
}

//
//FontResetCursor()
//
void FontResetCursor_FreeType( FreeTypeFont *freeFont)
{
    //code
    freeFont->cursorPos[0] = 0.0f;
    freeFont->cursorPos[1] = 0.0f;
}

//
//FontSetCursor()
//
void FontSetCursor_FreeType( FreeTypeFont *freeFont, float x, float y)
{
    //code
    freeFont->cursorPos[0] = x;
    freeFont->cursorPos[1] = y;
}

//
//FontSetCursor()
//
void FontSetScale_FreeType( FreeTypeFont *freeFont, float scaleX, float scaleY)
{
    //code
    freeFont->fontScaleW = scaleX;
    freeFont->fontScaleH = scaleY;
}

//
//calculateWidth()
//
static int calculateWidth( FreeTypeFont *freeFont, char *str)
{
    //variable
    float w = 0;

    //code
    while( *str)
    {
        w = w + ( freeFont->characterMap[*str].advance >> 6) * freeFont->fontScaleW;
        str++;
    }
    return(w);
}

//
//FontGetLength_FreeType()
//
int FontGetLength_FreeType( FreeTypeFont *freeFont, const char *text)
{
    //varialble declarations
    int stringLength;
    int returnWidth = 0;
    int index;

    //code
    stringLength = strlen( text);

    for( int i = 0; i < stringLength; i++)
    {
        returnWidth = returnWidth + (freeFont->characterMap[text[i]].advance >> 6) * freeFont->fontScaleW;  //bitshift by 6 to get value in pixels ( 2 ^ 6 = 64)   //Divide amount of 1/64th by 64 to get amount of pixels
    }

    return( returnWidth);
}

//
//FontCursorPrintText2D_FreeType()
//
void FontCursorPrintText2D_FreeType( FreeTypeFont *freeFont, const char *text, int viewportWidth, int viewportHeight)
{
    //variable declarations
    size_t temp_length = strlen( text);
    char *temp = (char *) calloc(1,  temp_length + 2);
    if( temp == NULL)
    {
        fprintf( gpLogFile, "Memory Allocation Failed\n");
        return;
    }
    strcpy( temp, text);
    temp[temp_length] = '\0';

    //code
     int currentPolygonMode;
    glGetIntegerv( GL_POLYGON_MODE, &currentPolygonMode);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

    /////////////////////////////////////////
    unsigned int length;

    float vertices[12];
    float uvs[12];

    float currentWidth = freeFont->cursorPos[0];

    char *token = strtok((char*)temp, " ");

    glEnable( GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram( text2D_program);
        //Bind Texture
        glActiveTexture( GL_TEXTURE0);
        glUniform1i( uniform_text_TextSampler, 0);

        glUniform2f( uniform_text_ScreenSize, (float)viewportWidth, (float)viewportHeight);
        glUniform4f( uniform_text_Color, freeFont->textColor[0], freeFont->textColor[1], freeFont->textColor[2], freeFont->textColor[3]);


        glBindVertexArray( vao_text);

            while( token)
            {
                length = strlen( token);
                int stringWidth = calculateWidth( freeFont, token);
                
                currentWidth = currentWidth + stringWidth;
                if( currentWidth > (viewportWidth - 8))
                {
                    freeFont->cursorPos[0] = freeFont->newLineXPos;
                    freeFont->cursorPos[1] = freeFont->cursorPos[1] - freeFont->fontHeight * freeFont->fontScaleH;
                    currentWidth = stringWidth;
                }

                for( int i = 0; i < length; ++i)
                {
                    struct CHARACTER ch = freeFont->characterMap[token[i]];
                    float xpos = freeFont->cursorPos[0] + ch.bearing.x * freeFont->fontScaleW;
                    float ypos = freeFont->cursorPos[1] - (ch.size.y - ch.bearing.y) * freeFont->fontScaleH;

                    float w = ch.size.x * freeFont->fontScaleW;
                    float h = ch.size.y * freeFont->fontScaleH;

                    ///////POSITION
                    vertices[ 2 * 0 + 0] = xpos;        vertices[ 2 * 0 + 1] = ypos + h;
                    vertices[ 2 * 1 + 0] = xpos;        vertices[ 2 * 1 + 1] = ypos;
                    vertices[ 2 * 2 + 0] = xpos + w;    vertices[ 2 * 2 + 1] = ypos;
                    
                    vertices[ 2 * 3 + 0] = xpos;        vertices[ 2 * 3 + 1] = ypos + h;
                    vertices[ 2 * 4 + 0] = xpos + w;    vertices[ 2 * 4 + 1] = ypos;
                    vertices[ 2 * 5 + 0] = xpos + w;    vertices[ 2 * 5 + 1] = ypos + h;

                    ///////UV
                    uvs[ 2 * 0 + 0] = 0.0f;     uvs[ 2 * 0 + 1] = 0.0f;
                    uvs[ 2 * 1 + 0] = 0.0f;     uvs[ 2 * 1 + 1] = 1.0f;
                    uvs[ 2 * 2 + 0] = 1.0f;     uvs[ 2 * 2 + 1] = 1.0f;
                    
                    uvs[ 2 * 3 + 0] = 0.0f;     uvs[ 2 * 3 + 1] = 0.0f;
                    uvs[ 2 * 4 + 0] = 1.0f;     uvs[ 2 * 4 + 1] = 1.0f;
                    uvs[ 2 * 5 + 0] = 1.0f;     uvs[ 2 * 5 + 1] = 0.0f;


                    freeFont->cursorPos[0] = freeFont->cursorPos[0] + (ch.advance >> 6) * freeFont->fontScaleW; //bitshift by 6 to get value in pixels ( 2 ^ 6 = 64)   //Divide amount of 1/64th by 64 to get amount of pixels

                    //vertex data
                    glBindBuffer( GL_ARRAY_BUFFER, vbo_textVertexBuffer);
                    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                    //uv data
                    glBindBuffer( GL_ARRAY_BUFFER, vbo_textUVBuffer);
                    glBufferData( GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

                    glBindTexture( GL_TEXTURE_2D, ch.textureID);
                    glDrawArrays( GL_TRIANGLES, 0, 6);

                }

                struct CHARACTER ch = freeFont->characterMap[' '];
                float xpos = freeFont->cursorPos[0] + ch.bearing.x * freeFont->fontScaleW;
                float ypos = freeFont->cursorPos[1] - (ch.size.y - ch.bearing.y) * freeFont->fontScaleH;

                float w = ch.size.x * freeFont->fontScaleW;
                float h = ch.size.y * freeFont->fontScaleH;

                ///////POSITION
                vertices[ 2 * 0 + 0] = xpos;        vertices[ 2 * 0 + 1] = ypos + h;
                vertices[ 2 * 1 + 0] = xpos;        vertices[ 2 * 1 + 1] = ypos;
                vertices[ 2 * 2 + 0] = xpos + w;    vertices[ 2 * 2 + 1] = ypos;
                
                vertices[ 2 * 3 + 0] = xpos;        vertices[ 2 * 3 + 1] = ypos + h;
                vertices[ 2 * 4 + 0] = xpos + w;    vertices[ 2 * 4 + 1] = ypos;
                vertices[ 2 * 5 + 0] = xpos + w;    vertices[ 2 * 5 + 1] = ypos + h;

                ///////UV
                uvs[ 2 * 0 + 0] = 0.0f;     uvs[ 2 * 0 + 1] = 0.0f;
                uvs[ 2 * 1 + 0] = 0.0f;     uvs[ 2 * 1 + 1] = 1.0f;
                uvs[ 2 * 2 + 0] = 1.0f;     uvs[ 2 * 2 + 1] = 1.0f;
                
                uvs[ 2 * 3 + 0] = 0.0f;     uvs[ 2 * 3 + 1] = 0.0f;
                uvs[ 2 * 4 + 0] = 1.0f;     uvs[ 2 * 4 + 1] = 1.0f;
                uvs[ 2 * 5 + 0] = 1.0f;     uvs[ 2 * 5 + 1] = 0.0f;

                //vertex data
                glBindBuffer( GL_ARRAY_BUFFER, vbo_textVertexBuffer);
                glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                //uv data
                glBindBuffer( GL_ARRAY_BUFFER, vbo_textUVBuffer);
                glBufferData( GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

                glBindTexture( GL_TEXTURE_2D, ch.textureID);

                glDrawArrays( GL_TRIANGLES, 0, 6);


                freeFont->cursorPos[0] = freeFont->cursorPos[0] + (ch.advance >> 6) * freeFont->fontScaleW;
                currentWidth = currentWidth + (ch.advance >> 6) * freeFont->fontScaleW;

                token = strtok( NULL, " ");
            }
        /////////////////////////////////////////

            glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray( 0);
    glUseProgram( 0);

    glDisable( GL_BLEND);

    free( temp);
    temp = NULL;
    

    glPolygonMode( GL_FRONT_AND_BACK, currentPolygonMode);
}


//
//FontCursorPrintSingleLineText2D_FreeType()
//
void FontCursorPrintSingleLineText2D_FreeType( FreeTypeFont *freeFont, const char *text, int viewportWidth, int viewportHeight)
{
    //variable declarations
    unsigned int stringLength = strlen(text);

    float vertices[12];
    float uvs[12];

    float currentWidth = freeFont->cursorPos[0];

    //code
     int currentPolygonMode;
    glGetIntegerv( GL_POLYGON_MODE, &currentPolygonMode);

    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);

    /////////////////////////////////////////

    glEnable( GL_BLEND);
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram( text2D_program);
        //Bind Texture
        glActiveTexture( GL_TEXTURE0);
        glUniform1i( uniform_text_TextSampler, 0);

        glUniform2f( uniform_text_ScreenSize, (float)viewportWidth, (float)viewportHeight);
        glUniform4f( uniform_text_Color, freeFont->textColor[0], freeFont->textColor[1], freeFont->textColor[2], freeFont->textColor[3]);

        glBindVertexArray( vao_text);

                for( int i = 0; i < stringLength; ++i)
                {
                    struct CHARACTER ch = freeFont->characterMap[ text[i]];

                    float xpos = freeFont->cursorPos[0] + ch.bearing.x * freeFont->fontScaleW;
                    float ypos = freeFont->cursorPos[1] - (ch.size.y - ch.bearing.y) * freeFont->fontScaleH;

                    float w = ch.size.x * freeFont->fontScaleW;
                    float h = ch.size.y * freeFont->fontScaleH;

                    ///////POSITION
                    vertices[ 2 * 0 + 0] = xpos;        vertices[ 2 * 0 + 1] = ypos + h;
                    vertices[ 2 * 1 + 0] = xpos;        vertices[ 2 * 1 + 1] = ypos;
                    vertices[ 2 * 2 + 0] = xpos + w;    vertices[ 2 * 2 + 1] = ypos;
                    
                    vertices[ 2 * 3 + 0] = xpos;        vertices[ 2 * 3 + 1] = ypos + h;
                    vertices[ 2 * 4 + 0] = xpos + w;    vertices[ 2 * 4 + 1] = ypos;
                    vertices[ 2 * 5 + 0] = xpos + w;    vertices[ 2 * 5 + 1] = ypos + h;

                    ///////UV
                    uvs[ 2 * 0 + 0] = 0.0f;     uvs[ 2 * 0 + 1] = 0.0f;
                    uvs[ 2 * 1 + 0] = 0.0f;     uvs[ 2 * 1 + 1] = 1.0f;
                    uvs[ 2 * 2 + 0] = 1.0f;     uvs[ 2 * 2 + 1] = 1.0f;
                    
                    uvs[ 2 * 3 + 0] = 0.0f;     uvs[ 2 * 3 + 1] = 0.0f;
                    uvs[ 2 * 4 + 0] = 1.0f;     uvs[ 2 * 4 + 1] = 1.0f;
                    uvs[ 2 * 5 + 0] = 1.0f;     uvs[ 2 * 5 + 1] = 0.0f;


                    freeFont->cursorPos[0] = freeFont->cursorPos[0] + (ch.advance >> 6) * freeFont->fontScaleW; //bitshift by 6 to get value in pixels ( 2 ^ 6 = 64)   //Divide amount of 1/64th by 64 to get amount of pixels

                    //vertex data
                    glBindBuffer( GL_ARRAY_BUFFER, vbo_textVertexBuffer);
                    glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

                    //uv data
                    glBindBuffer( GL_ARRAY_BUFFER, vbo_textUVBuffer);
                    glBufferData( GL_ARRAY_BUFFER, sizeof(uvs), uvs, GL_STATIC_DRAW);

                    glBindTexture( GL_TEXTURE_2D, ch.textureID);
                    glDrawArrays( GL_TRIANGLES, 0, 6);

                }


            glBindTexture(GL_TEXTURE_2D, 0);

        glBindVertexArray( 0);
    glUseProgram( 0);

    glDisable( GL_BLEND);
    
    glPolygonMode( GL_FRONT_AND_BACK, currentPolygonMode);
}

//
//FontDelete()
//
void FontDelete( FreeTypeFont **freeFont)
{
    if( freeFont && *freeFont)
    {
        (*freeFont)->Delete();
        free( *freeFont);
        (*freeFont) = NULL;
    }
}


//
//FontUninitialize()
//
void FontRenderingUninitialize_FreeType( void)
{

    if(text2D_program )
    {
        DeleteProgram( text2D_program);
        text2D_program = 0;
    }

    DELETE_BUFFER( vbo_textUVBuffer);
    DELETE_BUFFER( vbo_textVertexBuffer);

    DELETE_VERTEX_ARRAY( vao_text);
}

