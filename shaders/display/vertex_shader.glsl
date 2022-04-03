#version 450 core

in vec2 vPosition;
in vec2 vTexCoord;

out vec2 uv;

void main( void)
{
    //code
    gl_Position = vec4( vPosition, 0.0, 1.0);
    uv = vTexCoord;
}
