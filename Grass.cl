
#define PI     3.14159265
#define TWO_PI 6.28318531

/* ___________________ structure declarations _____________________ */

#pragma pack(1)

typedef struct
{
    float position[3];
    float normal[3];
    float tangent[3];
    float texcoord[2];
} VERTEX;

typedef struct
{
    float position[3];
    float normal[3];
    float texcoord[2];
}GRASS_VERTEX;

typedef struct
{
    float m[4][4];
}Matrix4x4;


/* ___________________ global variable definition _____________________ */

__constant float grassBladeHeight = 0.83f;
__constant float grassBladeHeightRandom = 0.26f;
__constant float grassBladeWidth = 0.03f;
__constant float grassBladeWidthRandom = 0.01f;
__constant float grassBendRotationRandom = 0.4f;
__constant float grassBladeForwardAmount = 0.515f;  //0.815f
__constant float grassBladeCurvatureAmount = 1.18f;

__constant float2 windFrequency = (float2)( 0.05f, 0.05f );
__constant float2 windScale = (float2)( 0.009f, 0.009f );
__constant float2 windOffset = (float2)( 0.0f, 0.0f );
__constant float windStrength = 0.345f;

__constant int COLOR_CHANNELS = 4;


/* _______________________ function definition _________________________ */

Matrix4x4 RotationMatrix( float angle, float x, float y, float z)
{
    //code
    float s = sin( angle);
    float c = cos( angle);
    float t = 1.0 - c;
    
    Matrix4x4 rotMat;
    
    rotMat.m[0][0] = x*x*t +   c;   rotMat.m[1][0] =  y*x*t + z*s;    rotMat.m[2][0] =   x*z*t - y*s;   rotMat.m[3][0] =  0.0f;
    rotMat.m[0][1] = x*y*t - z*s;   rotMat.m[1][1] =  y*y*t +   c;    rotMat.m[2][1] =   y*z*t + x*s;   rotMat.m[3][1] =  0.0f;
    rotMat.m[0][2] = x*z*t + y*s;   rotMat.m[1][2] =  y*z*t - x*s;    rotMat.m[2][2] =   z*z*t +   c;   rotMat.m[3][2] =  0.0f;
    rotMat.m[0][3] =        0.0f;   rotMat.m[1][3] =         0.0f;    rotMat.m[2][3] =          0.0f;   rotMat.m[3][3] =  1.0f;
    
    return( rotMat);
}

float vjd_fract( float x)
{
    return( x - floor(x));
}

float vjd_random( float3 coord)
{
    float3 v = (float3)(12.9898f, 78.233f, 53.539f);
    return( vjd_fract( (float)sin( dot( coord, v) ) * 43758.5453f));
}

Matrix4x4 matMul( Matrix4x4 m1, Matrix4x4 m2)
{
    //variable declarations
    Matrix4x4 ret;
    float sum;

    //code
    
    /*
        [ m00 m01 m02 m03]  [ n00 n01 n02 n03]  
        [ m10 m11 m12 m13]  [ n10 n11 n12 n13]  
        [ m20 m21 m22 m23]  [ n20 n21 n22 n23]  
        [ m30 m31 m32 m33]  [ n30 n31 n32 n33]  
    */
    ret.m[0][0] = m1.m[0][0]*m2.m[0][0] + m1.m[0][1]*m2.m[1][0] + m1.m[0][2]*m2.m[2][0] + m1.m[0][3]*m2.m[3][0];
    ret.m[0][1] = m1.m[0][0]*m2.m[0][1] + m1.m[0][1]*m2.m[1][1] + m1.m[0][2]*m2.m[2][1] + m1.m[0][3]*m2.m[3][1];
    ret.m[0][2] = m1.m[0][0]*m2.m[0][2] + m1.m[0][1]*m2.m[1][2] + m1.m[0][2]*m2.m[2][2] + m1.m[0][3]*m2.m[3][2];
    ret.m[0][3] = m1.m[0][0]*m2.m[0][3] + m1.m[0][1]*m2.m[1][3] + m1.m[0][2]*m2.m[2][3] + m1.m[0][3]*m2.m[3][3];

    ret.m[1][0] = m1.m[1][0]*m2.m[0][0] + m1.m[1][1]*m2.m[1][0] + m1.m[1][2]*m2.m[2][0] + m1.m[1][3]*m2.m[3][0];
    ret.m[1][1] = m1.m[1][0]*m2.m[0][1] + m1.m[1][1]*m2.m[1][1] + m1.m[1][2]*m2.m[2][1] + m1.m[1][3]*m2.m[3][1];
    ret.m[1][2] = m1.m[1][0]*m2.m[0][2] + m1.m[1][1]*m2.m[1][2] + m1.m[1][2]*m2.m[2][2] + m1.m[1][3]*m2.m[3][2];
    ret.m[1][3] = m1.m[1][0]*m2.m[0][3] + m1.m[1][1]*m2.m[1][3] + m1.m[1][2]*m2.m[2][3] + m1.m[1][3]*m2.m[3][3];

    ret.m[2][0] = m1.m[2][0]*m2.m[0][0] + m1.m[2][1]*m2.m[1][0] + m1.m[2][2]*m2.m[2][0] + m1.m[2][3]*m2.m[3][0];
    ret.m[2][1] = m1.m[2][0]*m2.m[0][1] + m1.m[2][1]*m2.m[1][1] + m1.m[2][2]*m2.m[2][1] + m1.m[2][3]*m2.m[3][1];
    ret.m[2][2] = m1.m[2][0]*m2.m[0][2] + m1.m[2][1]*m2.m[1][2] + m1.m[2][2]*m2.m[2][2] + m1.m[2][3]*m2.m[3][2];
    ret.m[2][3] = m1.m[2][0]*m2.m[0][3] + m1.m[2][1]*m2.m[1][3] + m1.m[2][2]*m2.m[2][3] + m1.m[2][3]*m2.m[3][3];

    ret.m[3][0] = m1.m[3][0]*m2.m[0][0] + m1.m[3][1]*m2.m[1][0] + m1.m[3][2]*m2.m[2][0] + m1.m[3][3]*m2.m[3][0];
    ret.m[3][1] = m1.m[3][0]*m2.m[0][1] + m1.m[3][1]*m2.m[1][1] + m1.m[3][2]*m2.m[2][1] + m1.m[3][3]*m2.m[3][1];
    ret.m[3][2] = m1.m[3][0]*m2.m[0][2] + m1.m[3][1]*m2.m[1][2] + m1.m[3][2]*m2.m[2][2] + m1.m[3][3]*m2.m[3][2];
    ret.m[3][3] = m1.m[3][0]*m2.m[0][3] + m1.m[3][1]*m2.m[1][3] + m1.m[3][2]*m2.m[2][3] + m1.m[3][3]*m2.m[3][3];


    // for( int j = 0; j < 4; j++)
    // {
    //     for( int i = 0; i < 4; i++)
    //     {
    //         sum = 0.0;
    //         for( int n = 0; n < 4; n++)
    //         {
    //             sum += m1.m[n][i] * m2.m[j][n];
    //         }

    //         ret.m[j][i] = sum;
    //     }
    // }

    return( ret);
}

float4 matVecMul( Matrix4x4 mat, float4 v)
{
    //variable declarations
    float4 ret = (float4)(0.0);

    //code
    /*
        [ m00 m01 m02 m03]  [ v00]  
        [ m10 m11 m12 m13]  [ v10]  
        [ m20 m21 m22 m23]  [ v20]  
        [ m30 m31 m32 m33]  [ v30]  
    */

    ret.x = mat.m[0][0] * v.x + mat.m[1][0] * v.y + mat.m[2][0] * v.z + mat.m[3][0] * v.w;
    ret.y = mat.m[0][1] * v.x + mat.m[1][1] * v.y + mat.m[2][1] * v.z + mat.m[3][1] * v.w;
    ret.z = mat.m[0][2] * v.x + mat.m[1][2] * v.y + mat.m[2][2] * v.z + mat.m[3][2] * v.w;
    ret.w = mat.m[0][3] * v.x + mat.m[1][3] * v.y + mat.m[2][3] * v.z + mat.m[3][3] * v.w;

    // for( int i = 0; i < 4; i++)
    // {
    //     for( int j = 0; j < 4; j++)
    //     {
    //         ret[j] += mat.m[i][j] * v[i];
    //     }
    // }

    return( ret);
}



float4 getTexel( float2 uv, __global float *imageData, int width, int height)
{
    //code
    int x = floor( vjd_fract(uv.x) * ( width - 1));
    int y = floor( vjd_fract(uv.y) * ( height - 1));

    float4 sample = (float4)( 0.0, 0.0, 0.0, 0.0);

    sample.x = *(imageData + (COLOR_CHANNELS * ( y * width + x) + 0));
    sample.y = *(imageData + (COLOR_CHANNELS * ( y * width + x) + 1));
    sample.z = *(imageData + (COLOR_CHANNELS * ( y * width + x) + 2));
    sample.w = *(imageData + (COLOR_CHANNELS * ( y * width + x) + 3));

    return( sample);
}


// float remap( float s, float a1, float a2, float b1, float b2)
// {
//     return( b1 + (s - a1)*(b2-b1) / (a2-a1));
// }


__kernel void grass_kernel( 
    __global GRASS_VERTEX *outGrassData,       //out buffer                                     [ __OUT__ ]
    __global VERTEX       *vertices,           //vertices information                           [ __IN__ ]
    unsigned int           mesh_width,         //mesh width                                     [ __IN__ ]
    unsigned int           mesh_height,        //mesh height                                    [ __IN__ ]
    unsigned int           grassBladeSegment,  //segment per grass blade                        [ __IN__ ]
    __global float        *distortionMapData,  //distortion map normalized data [0.0 - 1.0]     [ __IN__ ]
             int           map_width,          //distortion map width                           [ __IN__ ]
             int           map_height,         //distortion map height                          [ __IN__ ]
             float         time                //animation time                                 [ __IN__ ]
)
{
    //variable declarations
    const int verticesPerBlade = 2 * grassBladeSegment;

    unsigned int x = get_global_id(0);
    unsigned int y = get_global_id(1);

    if( x >= mesh_width || y >= mesh_height)
        return;

    int index = y * mesh_width + x;
    float angle;
    float3 position, normal, tangent;
    //Matrix4x4 tangentToLocalMatrix, facingRotationMatrix, bendRotationMatrix;

    //code
    position = (float3) (vertices[index].position[0], vertices[index].position[1], vertices[index].position[2]);
    normal = (float3) (vertices[index].normal[0], vertices[index].normal[1], vertices[index].normal[2]);
    tangent = (float3) (vertices[index].tangent[0], vertices[index].tangent[1], vertices[index].tangent[2]);

    float3 biNormal = cross( normal, tangent);

    Matrix4x4 tangentToLocalMatrix;
    
    tangentToLocalMatrix.m[0][0] = tangent.x;      tangentToLocalMatrix.m[1][0] = biNormal.x;      tangentToLocalMatrix.m[2][0] = normal.x;      tangentToLocalMatrix.m[3][0] = 0.0;
    tangentToLocalMatrix.m[0][1] = tangent.y;      tangentToLocalMatrix.m[1][1] = biNormal.y;      tangentToLocalMatrix.m[2][1] = normal.y;      tangentToLocalMatrix.m[3][1] = 0.0;
    tangentToLocalMatrix.m[0][2] = tangent.z;      tangentToLocalMatrix.m[1][2] = biNormal.z;      tangentToLocalMatrix.m[2][2] = normal.z;      tangentToLocalMatrix.m[3][2] = 0.0;
    tangentToLocalMatrix.m[0][3] =       0.0;      tangentToLocalMatrix.m[1][3] =        0.0;      tangentToLocalMatrix.m[2][3] =      0.0;      tangentToLocalMatrix.m[3][3] = 1.0;

    // Matrix4x4 tangentToLocalMatrix = 
    // {
    //     1.0, 0.0, 0.0, 0.0,
    //     0.0, 0.0, 1.0, 0.0,
    //     0.0, 1.0, 0.0, 0.0,
    //     0.0, 0.0, 0.0, 1.0
    // };

    //random rotation of vertex but constistent between frame
    // angle = vjd_random( vertices[index].position.xyz) * TWO_PI;

    angle = vjd_random( position.xyz) * PI;
    Matrix4x4 facingRotationMatrix = RotationMatrix( angle, 0.0, 0.0, 1.0);

    //rotate along X-axis
    angle = vjd_random( position.zzx) * grassBendRotationRandom * PI * 0.5;
    Matrix4x4 bendRotationMatrix = RotationMatrix( angle, -1.0, 0.0, 0.0);


    //Wind Effect
    float2 uv = position.xz * windScale + windOffset + windFrequency * time;
    float4 color = getTexel( uv, distortionMapData, map_width, map_height);
    float2 windSample = (color.xy * (float2)(2.0, 2.0) - (float2)(1.0, 1.0));// * windStrength;

    // float remap_value = remap( sin( 3.0*time*windFrequency.x), -1, 1, -87, 2);

    // float3 windNormal = ((color.xyz) * (float3)(2.0) - (float3)(1.0)) - (float3)( remap_value, remap_value, remap_value);
    // float3 windDirection = normalize( (float3)(windSample.x, windSample.y, 0.0) + windNormal );    

    float3 windDirection = normalize( (float3)(windSample.x, windSample.y, 0.0) );
    Matrix4x4 windTransformMatrix = RotationMatrix( PI * windSample.x, windDirection.x, windDirection.y, windDirection.z);


    float width  = ( vjd_random( position.xzy) * 2.0 - 1.0) * grassBladeWidthRandom + grassBladeWidth;
    float height = ( vjd_random( position.zyx) * 2.0 - 1.0) * grassBladeHeightRandom + grassBladeHeight;
    float forward =  vjd_random( position.yyz) * grassBladeForwardAmount;

    float t;
    float segmentWidth, segmentHeight, segmentForward;
    float4 tangentPoint, localPosition;
    float4 tangentNormal, localNormal;

    Matrix4x4 baseTransformationMatrix = matMul( facingRotationMatrix, tangentToLocalMatrix);
    Matrix4x4 transformationMatrix = matMul( matMul( matMul( bendRotationMatrix, facingRotationMatrix), windTransformMatrix), tangentToLocalMatrix);

    int vertexIndex;

    for( int i = 0; i < grassBladeSegment; i++)
    {

        Matrix4x4 M = ( i == 0) ? baseTransformationMatrix : transformationMatrix;
        t = (float)i / (float)(grassBladeSegment);

        segmentWidth = width * ( 1 - t);
        segmentHeight = height * t;
        //segmentForward = forward * t;
        segmentForward = pow( t, 4.0f * grassBladeCurvatureAmount) * forward;

        //////////////////////////////////////
        tangentPoint = (float4)( segmentWidth, segmentForward, segmentHeight, 0.0);
        localPosition = matVecMul( M, tangentPoint);
        localPosition.xyz =  localPosition.xyz + position;

        tangentNormal = (float4)( 0.0, -1.0, segmentForward, 0.0);
        // localNormal = matVecMul( M, (float4)(tangentNormal.xyz + windNormal, 0.0));
        localNormal = matVecMul( M, (float4)(tangentNormal.xyz, 0.0));

        vertexIndex = (verticesPerBlade * index) + (2 * i);

        outGrassData[ vertexIndex + 0].position[0] = localPosition.x;
        outGrassData[ vertexIndex + 0].position[1] = localPosition.y;
        outGrassData[ vertexIndex + 0].position[2] = localPosition.z;

        outGrassData[ vertexIndex + 0].normal[0] = localNormal.x;
        outGrassData[ vertexIndex + 0].normal[1] = localNormal.y;
        outGrassData[ vertexIndex + 0].normal[2] = localNormal.z;

        outGrassData[ vertexIndex + 0].texcoord[0] = 0.0;
        outGrassData[ vertexIndex + 0].texcoord[1] = t;


        //////////////////////////////////////
        tangentPoint = (float4)( -segmentWidth, segmentForward, segmentHeight, 0.0);
        localPosition = matVecMul( M, tangentPoint);
        localPosition.xyz = localPosition.xyz + position;

        outGrassData[ vertexIndex + 1].position[0] = localPosition.x;
        outGrassData[ vertexIndex + 1].position[1] = localPosition.y;
        outGrassData[ vertexIndex + 1].position[2] = localPosition.z;

        outGrassData[ vertexIndex + 1].normal[0] = localNormal.x;
        outGrassData[ vertexIndex + 1].normal[1] = localNormal.y;
        outGrassData[ vertexIndex + 1].normal[2] = localNormal.z;

        outGrassData[ vertexIndex + 1].texcoord[0] = 1.0;
        outGrassData[ vertexIndex + 1].texcoord[1] = t;

    }
}

