#version 450 core

in vec4 vPosition;
in vec2 vTexCoord;

out vec2 uv;

void main( void)
{
    uv = vTexCoord;

    gl_Position = vPosition;
}
