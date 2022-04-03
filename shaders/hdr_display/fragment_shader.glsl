#version 450 core

in vec2 uv;

uniform float exposure;
uniform sampler2D hdrBuffer;

out vec4 fragColor;

const float gamma = 2.2;

void main( void)
{
    vec3 hdrColor = texture( hdrBuffer, uv).rgb;

    //exposure
    vec3 mapped = vec3( 1.0) - exp( -hdrColor * exposure);

    //gamma correction
    mapped = pow( mapped, vec3( 1.0 / gamma));

    fragColor = vec4( 1.0, 0.0, 0.0, 1.0);
    fragColor = vec4( mapped, 1.0);






    // vec4 hdrColor = texture( hdrBuffer, uv);

    // //exposure
    // vec4 mapped = vec4( 1.0) - exp( -hdrColor * exposure);

    // //gamma correction
    // mapped = pow( mapped, vec4( 1.0 / gamma));

    // fragColor = mapped;
}

