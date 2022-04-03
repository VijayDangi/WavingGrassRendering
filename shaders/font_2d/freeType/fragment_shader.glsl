#version 450 core

in vec2 uv;

out vec4 fragColor;

uniform sampler2D TextSampler;
uniform vec4 Color;

void main( void)
{
    float alpha = texture( TextSampler, uv).r;
    fragColor = vec4( Color.xyz, alpha);
}
