#version 450 core

in vec2 uv;

out vec4 fragColor;

uniform sampler2D TextSampler;
uniform vec4 Color;

void main( void)
{
//    fragColor = vec4(Color.rgb * texture( TextSampler, uv).rgb, 1.0);
     fragColor = Color * texture( TextSampler, uv);
}
