#version 450 core

in vec2 uv;

uniform sampler2D sceneSampler;

out vec4 fragColor;

void main( void)
{
    //code
    fragColor = textureLod( sceneSampler, uv, 0.0);
}
