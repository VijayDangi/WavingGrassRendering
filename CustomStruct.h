#ifndef __CUSTOM_STRUCT_H__
#define __CUSTOM_STRUCT_H__

//============================== macro function
#define DELETE_VERTEX_ARRAY(vao) \
    if( vao)    \
    {   \
        glDeleteVertexArrays( 1, &vao); \
        vao = 0;    \
    }

#define DELETE_BUFFER(vbo) \
    if( vbo)    \
    {   \
        glDeleteBuffers( 1, &vbo); \
        vbo = 0;    \
    }

#define DELETE_TEXTURE(t) \
    if( t)    \
    {   \
        glDeleteTextures( 1, &t); \
        t = 0;  \
    }

//VERTEX ATTRIBUTE INDEX
enum
{
    VJD_ATTRIBUTE_POSITION = 0,
    VJD_ATTRIBUTE_COLOR,
    VJD_ATTRIBUTE_NORMAL,
    VJD_ATTRIBUTE_TEXTCOORD,
    VJD_ATTRIBUTE_TANGENT
};


//================================ OpenGL and Mesh STRUCT
    //bind data or object require to create Framebuffer
typedef struct FRAMEBUFFER
{
    GLuint framebuffer;
    GLuint colorAttachment;
    GLuint depthAttachment;
    GLuint stencilAttachment;

    int width;
    int height;

    FRAMEBUFFER()
    {
        this->framebuffer = 0;
        this->colorAttachment = 0;
        this->depthAttachment = 0;
        this->stencilAttachment = 0;

        this->width = 0;
        this->height = 0;
    }

    void Delete( void)
    {
        if(this->framebuffer)
        {
            glDeleteFramebuffers( 1, &this->framebuffer);
            this->framebuffer = 0;
        }

        DELETE_TEXTURE( this->colorAttachment);
        DELETE_TEXTURE( this->depthAttachment);
        DELETE_TEXTURE( this->stencilAttachment);

        this->width = 0;
        this->height = 0;
    }
}FRAMEBUFFER;

//  Mesh buffer data
typedef struct MeshBuffer
{
	GLuint vao;
	GLuint vbo;
	GLuint vbo_element;

	int vertexCount;
	int elementCount;

    GLenum elementType;

	MeshBuffer()
	{
		this->vao = 0;
		this->vbo = 0;
		this->vbo_element = 0;

		this->vertexCount = 0;
		this->elementCount = 0;
	}

	void Delete()
	{
		DELETE_BUFFER( this->vbo_element)
		DELETE_BUFFER( this->vbo)
		DELETE_VERTEX_ARRAY( this->vao)

		this->vertexCount = 0;
		this->elementCount = 0;
	}
} MeshBuffer;


typedef struct OBJECT_BUFFER
{
	GLuint vao;
	GLuint vbo_position;
	GLuint vbo_texture;
	GLuint vbo_normal;
	GLuint vbo_tangent;
	GLuint vbo_element;
	int vertex_count;
	int index_count;


	OBJECT_BUFFER()
	{
		this->vao = 0;
		this->vbo_position = 0;
		this->vbo_texture = 0;
		this->vbo_normal = 0;
		this->vbo_tangent = 0;
		this->vbo_element = 0;
		this->vertex_count = 0;
		this->index_count = 0;
	}

	void Delete()
	{
		this->vertex_count = 0;
		this->index_count = 0;

		DELETE_VERTEX_ARRAY(this->vao);
		DELETE_BUFFER(this->vbo_position);
		DELETE_BUFFER(this->vbo_texture);
		DELETE_BUFFER(this->vbo_normal);
		DELETE_BUFFER(this->vbo_tangent);
		DELETE_BUFFER(this->vbo_element);
	}

} OBJECT_BUFFER;


//====================================== Image data
typedef struct IMAGE_DATA
{
    int width;
    int height;
    int bytePerPixel;
    float *normalizeImageData;

    IMAGE_DATA()
    {
        width = 0;
        height = 0;
        bytePerPixel = 0;
        normalizeImageData = NULL;
    }

    void Delete()
    {
        if(normalizeImageData)
        {
            free(normalizeImageData);
            normalizeImageData = NULL;
        }

        bytePerPixel = 0;
        height = 0;
        width = 0;
    }

} IMAGE_DATA;


//====================================== MATH Struct
struct Vector2f
{
	union
	{
		float x;
		float u;
	};

	union
	{
		float y;
		float v;
	};

    Vector2f()
    {
        x = 0.0f;
        y = 0.0f;
    }

    void normalize()
    {
        float length = sqrt( x*x + y*y);
        x = x / length;
        y = y / length;
    }
};

struct Vector3f
{
    union{ float x; float r; };
    union{ float y; float g; };
    union{ float z; float b; };

    Vector3f()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

	void normalize()
	{
		float length = sqrt( x*x + y*y + z*z);
		x = x / length;
		y = y / length;
		z = z / length;
	}

	Vector3f operator+( Vector3f v)
	{
		Vector3f add;
		add.x = x + v.x;
		add.y = y + v.y;
		add.z = z + v.z;

		return( add);
	}

    Vector3f operator-( Vector3f v)
	{
		Vector3f sub;
		sub.x = x - v.x;
		sub.y = y - v.y;
		sub.z = z - v.z;

		return( sub);
	}

    Vector3f operator*( float a)
	{
		Vector3f mul;
		mul.x = x * a;
		mul.y = y * a;
		mul.z = z * a;

		return( mul);
	}

    Vector3f operator/( float a)
	{
		Vector3f div;
		div.x = x / a;
		div.y = y / a;
		div.z = z / a;

		return( div);
	}

	Vector3f cross( Vector3f v)
	{
		Vector3f result;

		result.x = y * v.z - z * v.y;
		result.y = z * v.x - x * v.z;
		result.z = x * v.y - y * v.x;

		return( result);
	}
};


//=========================================================================
//	KeyBoard
extern unsigned char g_keyboardState[256];		//Main.cpp

enum KeyState
{
	KEY_STATE_NONE = 0,
	KEY_STATE_HELD = 1 << 0,
	KEY_STATE_PRESSED = 1 << 1,
	KEY_STATE_RELEASED = 1 << 2
};

#endif
