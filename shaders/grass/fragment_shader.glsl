#version 450 core

in vec2 uv;
in vec3 normal;
in vec3 worldPosition;

out vec4 fragColor;

// uniform vec3 _TopColor;// = vec3( 0.5792569, 0.84599996, 0.3297231);
// uniform vec3 _BottomColor;// = vec3( 0.061297283, 0.378, 0.07151349);

uniform vec3 LightPosition[5];
uniform vec3 LightColor[5];
uniform float LightConstant[5];
uniform float LightLinear[5];
uniform float LightQuadratic[5];

uniform float TranslucentGain;

uniform sampler2D GrassBladeSample;
uniform sampler2D GrassBladeAlphaSample;

uniform bool useLight;


vec3 lerp( vec3 v0, vec3 v1, float t)
{
    return( (1 - t) * v0 + t * v1);
}

void main( void)
{
    //code
    vec3 lightIntensity = vec3( 0.0);

    if( useLight)
    {
        // vec3 N = normalize( gl_FrontFacing ? normal : -normal);
        vec3 N = normalize( normal);

        for( int i = 0; i < 5; i++)
        {
            vec3 L = normalize(LightPosition[i] - worldPosition);
            float NtoL = max( max( dot( N, L), 0.0) + TranslucentGain, 0.0);
            float d = length( LightPosition[i] - worldPosition);
            float attenuation = 1.0 / ( LightConstant[i] + LightLinear[i] * d + LightQuadratic[i] * d * d);
            lightIntensity += NtoL * LightColor[i] * attenuation;
        }
    }
    else
    {
        lightIntensity = vec3( 1.0);
    }

    vec3 color = texture( GrassBladeSample, uv).rgb;
    float alpha = texture( GrassBladeAlphaSample, uv).r;
    if( alpha < 0.2)
    {
        discard;
        return;
    }
    fragColor = vec4( color.rgb * lightIntensity , alpha);
  
}
