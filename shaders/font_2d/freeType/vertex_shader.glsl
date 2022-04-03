#version 450 core

//Input vertex data, different for all executions of this shader
in vec2 vPosition; //vertexPosition_screenspace;
in vec2 vTexCoord;

out vec2 uv;

uniform vec2 ScreenSize;

void main( void)
{
    //output position of the vertex, in clip space
    //map [0, w][, h] tp [-1, 1][-1, 1]
    vec2 vertexPosition_HomoneneousSpace = vPosition - ScreenSize/2;    //[0, w] => [-w/2, w/2], [0, h] => [-h/2, h/2]
    vertexPosition_HomoneneousSpace = vertexPosition_HomoneneousSpace / (ScreenSize/2); //[-1, 1]

    gl_Position = vec4( vertexPosition_HomoneneousSpace, 0, 1);

    //UV of the vertex
    uv = vTexCoord;
}
