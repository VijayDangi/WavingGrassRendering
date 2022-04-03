//
//  Fragment Shader
//

#version 450 core

in vec2 uv;

uniform vec3 Color;
uniform float Alpha;

uniform bool useTexture;
uniform sampler2D tex;

out vec4 fragColor;

void main( void)
{
    if( useTexture)
    {
        fragColor = texture( tex, uv);
        fragColor.a *= Alpha;
    }
    else
    {
        fragColor = vec4(Color, Alpha);
    }
}

