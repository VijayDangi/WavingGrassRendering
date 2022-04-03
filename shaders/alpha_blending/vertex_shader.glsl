//
//  Vertex Shader
//

#version 450 core

#define M_PI  3.14159265358979323846

in vec4 vPosition;
in vec2 vTexCoord;

// uniform float SemiMajorAxis;
// uniform float Eccentricity;
// uniform mat4 OrbitalMatrix;

// uniform float OrbitVertexCount;

uniform mat4 MVPMatrix;

out vec2 uv;

void main( void)
{
    // float theta = gl_VertexID / OrbitVertexCount * 2.0 * M_PI;
    // float radius = (SemiMajorAxis * ( 1 - Eccentricity * Eccentricity)) / ( 1 - Eccentricity * cos( theta + M_PI));
    
    // vec4 position;
    // position.x = radius * vPosition.x;
    // position.y = radius * vPosition.y;
    // position.z = 0.0;
    // position.w = 0.0;
    
    // position = position * OrbitalMatrix;
    
    // gl_Position = MVPMatrix * vec4(position.xyz, 1.0);
    
    uv = vPosition.xy * 0.5 + 0.5;
    gl_Position = MVPMatrix * vec4( vPosition.xyz, 1.0);
    //gl_PointSize = 3.0;
}
