#ifndef __MY_MATH_H__
#define __MY_MATH_H__

#include <assert.h>
#include "vmath.h"


//macro function
#define MIN( a, b) (((a) < (b)) ? (a) : (b))
#define MAX( a, b) (((a) > (b)) ? (a) : (b))


#define CLAMP( x, minVal, maxVal) (MIN( MAX( x, minVal), maxVal))
    //Linear Interpolation (LERP)
#define LERP(A, B, t) ((1.0f-t)*(A) + t*(B))

//AstronomicalUnit To KiloMeter
//1 AU => 149 597 870. 700 Km
#define AUToKM(val) (val * 149597870.700f)


//vmath extension - additional function
namespace vmath
{
    template <typename T, const int N, const int M>
    static inline vecN<T,N> operator*( const matNM<T,N,M>& mat, const vecN<T,M>& vec)
    {
        int n, m;
        vecN<T,N> result(T(0));

        for (m = 0; m < M; m++)
        {
            for (n = 0; n < N; n++)
            {
                result[n] += mat[m][n] * vec[m] ;
            }
        }

        return result;
    }
}


namespace mymath
{
    static const float PI = 3.14159265f;
    static const float TWO_PI = 6.28318531f;



    //
    //determinantMatrix4x4()
    //
    static float determinantMatrix4x4( vmath::mat4 m)
    {
        //variable
        float result, a, b, c, d;
        
        //code
        /*
            m00 m01 m02 m03
            m10 m11 m12 m13
            m20 m21 m22 m23
            m30 m31 m32 m33
        */
        a = m[0][0] * (
                        m[1][1] * ( m[2][2] * m[3][3] - m[2][3] * m[3][2])
                    - m[1][2] * ( m[2][1] * m[3][3] - m[2][3] * m[3][1])
                    + m[1][3] * ( m[2][1] * m[3][2] - m[2][2] * m[3][1])
                    );

        b = m[0][1] * (
                        m[1][0] * ( m[2][2] * m[3][3] - m[2][3] * m[3][2])
                    - m[1][2] * ( m[2][0] * m[3][3] - m[2][3] * m[3][0])
                    + m[1][3] * ( m[2][0] * m[3][2] - m[2][2] * m[3][0])
                    );

        c = m[0][2] * (
                        m[1][0] * ( m[2][1] * m[3][3] - m[2][3] * m[3][1])
                    - m[1][1] * ( m[2][0] * m[3][3] - m[2][3] * m[3][0])
                    + m[1][3] * ( m[2][0] * m[3][1] - m[2][1] * m[3][0])
                    );

        d = m[0][3] * (
                        m[1][0] * ( m[2][1] * m[3][2] - m[2][2] * m[3][1])
                    - m[1][1] * ( m[2][0] * m[3][2] - m[2][2] * m[3][0])
                    + m[1][2] * ( m[2][0] * m[3][1] - m[2][1] * m[3][0])
                    );

        result = a - b + c - d;
        
        return(result);
    }



    //
    //determinantMatrix3x3()
    //
    static float determinantMatrix3x3( float a00, float a01, float a02,
                                float a10, float a11, float a12,
                                float a20, float a21, float a22
    )
    {
        return(
            a00 * ( a11 * a22 - a12 * a21)
            - a01 * ( a10 * a22 - a12 * a20)
            + a02 * ( a10 * a21 - a11 * a20)
        );
    }


    //
    //inverseMatrix()
    //
    static vmath::mat4 inverseMatrix( vmath::mat4 m)
    {
        //variable
        vmath::mat4 invert = vmath::mat4::identity();
        //code
        float determinant = determinantMatrix4x4( m);
        
        if( determinant != 0)
        {
            /*
                m00 m01 m02 m03
                m10 m11 m12 m13
                m20 m21 m22 m23
                m30 m31 m32 m33
            */
            
            float determinant_invert = 1.0f/determinant;
            
            //first row
            float t00 = determinantMatrix3x3(
                            m[1][1], m[1][2], m[1][3],
                            m[2][1], m[2][2], m[2][3],
                            m[3][1], m[3][2], m[3][3]
                        );
            float t01 = - determinantMatrix3x3(
                            m[1][0], m[1][2], m[1][3],
                            m[2][0], m[2][2], m[2][3],
                            m[3][0], m[3][2], m[3][3]
                        );
            float t02 = determinantMatrix3x3(
                            m[1][0], m[1][1], m[1][3],
                            m[2][0], m[2][1], m[2][3],
                            m[3][0], m[3][1], m[3][3]
                        );
            float t03 = - determinantMatrix3x3(
                            m[1][0], m[1][1], m[1][2],
                            m[2][0], m[2][1], m[2][2],
                            m[3][0], m[3][1], m[3][2]
                        );

            //second row
            float t10 = - determinantMatrix3x3(
                            m[0][1], m[0][2], m[0][3],
                            m[2][1], m[2][2], m[2][3],
                            m[3][1], m[3][2], m[3][3]
                        );
            float t11 = determinantMatrix3x3(
                            m[0][0], m[0][2], m[0][3],
                            m[2][0], m[2][2], m[2][3],
                            m[3][0], m[3][2], m[3][3]
                        );
            float t12 = - determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][3],
                            m[2][0], m[2][1], m[2][3],
                            m[3][0], m[3][1], m[3][3]
                        );
            float t13 = determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][2],
                            m[2][0], m[2][1], m[2][2],
                            m[3][0], m[3][1], m[3][2]
                        );

            //third row
            float t20 = determinantMatrix3x3(
                            m[0][1], m[0][2], m[0][3],
                            m[1][1], m[1][2], m[1][3],
                            m[3][1], m[3][2], m[3][3]
                        );
            float t21 = - determinantMatrix3x3(
                            m[0][0], m[0][2], m[0][3],
                            m[1][0], m[1][2], m[1][3],
                            m[3][0], m[3][2], m[3][3]
                        );
            float t22 = determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][3],
                            m[1][0], m[1][1], m[1][3],
                            m[3][0], m[3][1], m[3][3]
                        );
            float t23 = - determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][2],
                            m[1][0], m[1][1], m[1][2],
                            m[3][0], m[3][1], m[3][2]
                        );

            //fourth row
            float t30 = - determinantMatrix3x3(
                            m[0][1], m[0][2], m[0][3],
                            m[1][1], m[1][2], m[1][3],
                            m[2][1], m[2][2], m[2][3]
                        );
            float t31 = determinantMatrix3x3(
                            m[0][0], m[0][2], m[0][3],
                            m[1][0], m[1][2], m[1][3],
                            m[2][0], m[2][2], m[2][3]
                        );
            float t32 = - determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][3],
                            m[1][0], m[1][1], m[1][3],
                            m[2][0], m[2][1], m[2][3]
                        );
            float t33 = determinantMatrix3x3(
                            m[0][0], m[0][1], m[0][2],
                            m[1][0], m[1][1], m[1][2],
                            m[2][0], m[2][1], m[2][2]
                        );



            //transpose and dive by determinant_invert
            invert[0][0] = t00 * determinant_invert;
            invert[0][1] = t10 * determinant_invert;
            invert[0][2] = t20 * determinant_invert;
            invert[0][3] = t30 * determinant_invert;
            
            invert[1][0] = t01 * determinant_invert;
            invert[1][1] = t11 * determinant_invert;
            invert[1][2] = t21 * determinant_invert;
            invert[1][3] = t31 * determinant_invert;
    
            invert[2][0] = t02 * determinant_invert;
            invert[2][1] = t12 * determinant_invert;
            invert[2][2] = t22 * determinant_invert;
            invert[2][3] = t32 * determinant_invert;
            
            invert[3][0] = t03 * determinant_invert;
            invert[3][1] = t13 * determinant_invert;
            invert[3][2] = t23 * determinant_invert;
            invert[3][3] = t33 * determinant_invert;
            
        }
        else
        {
            invert = vmath::mat4::identity();
        }
        
        return(invert);
    }


    //
    //quaternionFromMatrix()
    //
    static vmath::quaternion quaternionFromMatrix(vmath::mat4 matrix)
    {
        //code
            //___________________ FROM Thin Matrix Tutorial ______________________
        float x, y, z, w;
        float diagonal = matrix[0][0] + matrix[1][1] + matrix[2][2];

        if (diagonal > 0)
        {
            float w4 = (float)(sqrt(diagonal + 1.0f) * 2.0f);
            w = w4 / 4.0f;
            x = (matrix[2][1] - matrix[1][2]) / w4;
            y = (matrix[0][2] - matrix[2][0]) / w4;
            z = (matrix[1][0] - matrix[0][1]) / w4;
        }
        else if ((matrix[0][0] > matrix[1][1]) && (matrix[0][0] > matrix[2][2]))
        {
            float x4 = (float)(sqrt( 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2]) * 2.0f);
            w = (matrix[2][1] - matrix[1][2]) / x4;
            x = x4 / 4.0f;
            y = (matrix[0][1] + matrix[1][0]) / x4;
            z = (matrix[0][2] + matrix[2][0]) / x4;
        }
        else if ( matrix[1][1] > matrix[2][2])
        {
            float y4 = (float)(sqrt(1.0f - matrix[0][0] + matrix[1][1] - matrix[2][2]) * 2.0f);
            w = (matrix[0][2] - matrix[2][0]) / y4;
            x = (matrix[0][1] + matrix[1][0]) / y4;
            y = y4 / 4.0f; 
            z = (matrix[1][2] + matrix[2][1]) / y4;
        }
        else
        {
            float z4 = (float)(sqrt(1.0f - matrix[0][0] - matrix[1][1] + matrix[2][2]) * 2.0f);
            w = (matrix[1][0] - matrix[0][1]) / z4;
            x = (matrix[0][2] + matrix[2][0]) / z4;
            y = (matrix[1][2] + matrix[2][1]) / z4;
            z = z4 / 4.0f;
        }

        return(vmath::quaternion(x, y, z, w));


        /*	
            //___________________ FROM glm Library ______________________

        float fourXSquareMinus1 = matrix[0][0] - matrix[1][1] - matrix[2][2];
        float fourYSquareMinus1 = matrix[1][1] - matrix[0][0] - matrix[2][2];
        float fourZSquareMinus1 = matrix[2][2] - matrix[0][0] - matrix[1][1];
        float fourWSquareMinus1 = matrix[0][0] + matrix[1][1] + matrix[2][2];

        int biggestIndex = 0;
        float fourBiggestSquareMinus1 = fourWSquareMinus1;

        if (fourXSquareMinus1 > fourBiggestSquareMinus1)
        {
            fourBiggestSquareMinus1 = fourXSquareMinus1;
            biggestIndex = 1;
        }
        if (fourYSquareMinus1 > fourBiggestSquareMinus1)
        {
            fourBiggestSquareMinus1 = fourYSquareMinus1;
            biggestIndex = 2;
        }
        if (fourZSquareMinus1 > fourBiggestSquareMinus1)
        {
            fourBiggestSquareMinus1 = fourZSquareMinus1;
            biggestIndex = 3;
        }

        float biggestVal = sqrt(fourBiggestSquareMinus1 + 1.0) * 0.5;
        float mult = 0.25 / biggestVal;

        float x, y, z, w;

        switch (biggestIndex)
        {
        case 0:
            x = biggestVal;
            y = (matrix[1][2] - matrix[2][1]) * mult;
            z = (matrix[2][0] - matrix[0][2]) * mult;
            w = (matrix[0][1] - matrix[1][0]) * mult;
            break;

        case 1:
            x = (matrix[1][2] - matrix[2][1]) * mult;
            y = biggestVal;
            z = (matrix[0][1] + matrix[1][0]) * mult;
            w = (matrix[2][0] + matrix[0][2]) * mult;
            break;

        case 2:
            x = (matrix[2][0] - matrix[0][2]) * mult;
            y = (matrix[0][1] + matrix[1][0]) * mult;
            z = biggestVal;
            w = (matrix[1][2] + matrix[2][1]) * mult;
            break;

        case 3:
            x = (matrix[0][1] - matrix[1][0]) * mult;
            y = (matrix[2][0] + matrix[0][2]) * mult;
            z = (matrix[1][2] + matrix[2][1]) * mult;
            w = biggestVal;
            break;

        default:
            //This is just to silence a -Wswitch-default warning in GCC. Should never actually get here.
            assert(false);
            x = y = z = w = 0;
        }

        return(vmath::quaternion(x, y, z, w));*/
    }

    static vmath::vec3  quaternionToEulerAngles(vmath::quaternion q)
    {
        //code
        vmath::vec3 angles;

        //roll (x-axis rotation)
        double sinr_cosp = 2.0 * (q[3] * q[0] + q[1] * q[2]);
        double cosr_cosp = 1.0 - 2.0 * (q[0] * q[0] + q[1] * q[1]);
        angles[0] = atan2(sinr_cosp, cosr_cosp);

        //pitch (y-axis rotation)
        double sinp = 2.0 * (q[3] * q[1] - q[2] * q[0]);
        if (fabs(sinp) >= 1.0)
            angles[1] = copysign(M_PI / 2.0, sinp);	//use 90 degrees if out of range
        else
            angles[1] = asin(sinp);

        //yaw (z-axis rotation(
        double siny_cosp = 2.0 * (q[3] * q[2] + q[0] * q[1]);
        double cosy_cosp = 1.0 - 2.0 * (q[1] * q[1] + q[2] * q[2]);
        angles[2] = atan2(siny_cosp, cosy_cosp);

        return(angles);
    }


    //
    //quaternionInterpolate
    //
    static vmath::quaternion quaternionInterpolate(vmath::quaternion a, vmath::quaternion b, float blend)
    {
        //code
        vmath::quaternion temp = vmath::quaternion(0, 0, 0, 1);

        float dot = a[3] * b[3] +
                    a[0] * b[0] +
                    a[1] * b[1] +
                    a[2] * b[2];

        float blendI = 1.0f - blend;

        if (dot < 0)
        {
            temp[3] = blendI * a[3] + blend * -b[3];
            temp[0] = blendI * a[0] + blend * -b[0];
            temp[1] = blendI * a[1] + blend * -b[1];
            temp[2] = blendI * a[2] + blend * -b[2];
        }
        else
        {
            temp[3] = blendI * a[3] + blend * b[3];
            temp[0] = blendI * a[0] + blend * b[0];
            temp[1] = blendI * a[1] + blend * b[1];
            temp[2] = blendI * a[2] + blend * b[2];
        }
        
        vmath::quaternion result = vmath::normalize(temp);
        return(result);
    }


    //
    //smoothstep
    //
    static float smoothstep(float edge0, float edge1, float x)
    {
        float val = (x - edge0) / (edge1 - edge0);  //inverse linear interpolation
    
        float t = CLAMP( val, 0.0f, 1.0f);
        return( t * t * ( 3.0f - 2.0f * t));
    }

    //
    //fractional part
    //
    static float fract( float a)
    {
        return( a - floor(a));
    }
}

#endif
