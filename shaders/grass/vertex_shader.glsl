#version 450 core

in vec3 vPosition;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vTexCoord;

uniform mat4 MMatrix;
uniform mat4 VMatrix;
uniform mat4 PMatrix;

out vec2 uv;
out vec3 normal;
out vec3 worldPosition;

void main( void)
{
    //code
    uv = vTexCoord;
    normal = mat3( MMatrix) * vNormal;

    worldPosition = (MMatrix * vec4( vPosition, 1.0)).xyz;

    gl_Position = PMatrix * VMatrix * MMatrix * vec4( vPosition, 1.0);
}
