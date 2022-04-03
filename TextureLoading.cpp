#include "Main.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image/stb_image.h"

 //
 //LoadTexture()
 //
GLuint LoadTexture( char *fileName)
{
	//variable declaration
	GLuint textureID = 0;
	int width, height, channels;
	unsigned char *imageData;
	int components;
	int eFormat;

	//code
	imageData = stbi_load( fileName, &width, &height, &channels, 0);

	if(imageData)
	{
		if( channels == 3)
		{
			components = GL_RGB;
			eFormat = GL_RGB;
		}
		else if( channels == 4)
		{
			components = GL_RGBA;
			eFormat = GL_RGBA;
		}

		//pixel storage
		glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

		//generate (allocation) texture memory in GPU memory
		glGenTextures( 1, &textureID);

		glBindTexture( GL_TEXTURE_2D, textureID);

		//Setting texture parameter
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		glTexImage2D( GL_TEXTURE_2D, 0, components, width, height, 0, eFormat, GL_UNSIGNED_BYTE, imageData);

		glGenerateMipmap( GL_TEXTURE_2D);

		glBindTexture( GL_TEXTURE_2D, 0);

		stbi_image_free( imageData);
		fprintf( gpLogFile, "loadTexture(): %d\n", textureID);
	}
	else
	{
		fprintf( gpLogFile, "Cannot load \"%s\"\n", fileName);
	}

	return(textureID);
}



