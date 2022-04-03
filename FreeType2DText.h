#ifndef __TEXT_RENDERING_FREETYPE_H__
#define __TEXT_RENDERING_FREETYPE_H__


struct FreeTypeFont;

GLuint loadTexture( char *fileName);    //OGL.cpp

void FontRendringInitialize_FreeType( void);
FreeTypeFont* FontCreate( const char *fileName, int fontSize);

float FontGetSize( FreeTypeFont *freeFont);
void FontSetColor_FreeType( FreeTypeFont *freeFont,float r, float g, float b, float a);
void FontNewLine_FreeType( FreeTypeFont *freeFont);
void FontSetNewLineXPosition_FreeType( FreeTypeFont *freeFont,float x);
int FontGetLength_FreeType( FreeTypeFont *freeFont,const char *text);
void FontResetCursor_FreeType( FreeTypeFont *freeFont);
void FontSetCursor_FreeType( FreeTypeFont *freeFont,float x, float y);
void FontSetScale_FreeType( FreeTypeFont *freeFont,float scaleX, float scaleY);
void FontCursorPrintText2D_FreeType( FreeTypeFont *freeFont,const char *text, int viewportWidth, int viewportHeight);
void FontCursorPrintSingleLineText2D_FreeType( FreeTypeFont *freeFont,const char *text, int viewportWidth, int viewportHeight);

void FontDelete( FreeTypeFont **freeFont);
void FontRenderingUninitialize_FreeType( void);

#endif