#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "Geometry.h"

struct Vector3f
{
	float x, y, z;

	void normalize(void)
	{
		float length = sqrt(x * x + y * y + z * z);

		if (length == 0.0f)
		{
			x = y = z = 0.0;
		}
		else
		{
			x = x / length;
			y = y / length;
			z = z / length;
		}
	}
};

struct Vector2f
{
	float x, y;
};


//
//CreateSphere()
//
void CreateSphere( float radius, int slices, int stacks,  Geometry *sphere)
{
    //variable declarations
    float du = 2 * M_PI / (float)slices;
    float dv = M_PI / (float)stacks;

    float x, y, z;
    float u, v;
    int i, j;
    int indexV, indexI;

    int vertex_count;

    //code
    if(radius <= 0.0f)
        radius = 0.5f;

    if(slices <= 0)
        slices = 32;

    if(stacks <= 0)
        stacks = 16;

    //Allocate memory
    vertex_count = (slices + 1) * (stacks + 1);

    sphere->positions = (float*) calloc(1, 3 * vertex_count * sizeof(float));
    sphere->normals = (float*) calloc(1, 3 * vertex_count * sizeof(float));
    sphere->textures = (float*) calloc(1, 2 * vertex_count * sizeof(float));
    sphere->tangent = NULL;

    sphere->indices = (unsigned int*) calloc(1, 2 * slices * stacks * 3 * sizeof(float));

        //fill arrays
    indexV = 0;
    for( i = 0; i <= stacks; i++)
    {
        v = - M_PI/2.0f + i*dv;

        for( j = 0; j <= slices; j++)
        {
            u = j*du;
            x = cos(u) * cos(v);
            y = sin(u) * cos(v);
            z = sin(v);

            sphere->positions[3*indexV + 0] = radius * x;
            sphere->positions[3*indexV + 1] = radius * y;
            sphere->positions[3*indexV + 2] = radius * z;

            sphere->normals[3*indexV + 0] = x;
            sphere->normals[3*indexV + 1] = y;
            sphere->normals[3*indexV + 2] = z;

            sphere->textures[2*indexV + 0] = (float)j/(float)slices;
            sphere->textures[2*indexV + 1] = (float)i/(float)stacks;

            indexV++;
        }
    }


    indexI = 0;
    for( j = 0; j < stacks; j++)
    {
        unsigned int row_one = j * (slices + 1);
        unsigned int row_two = (j + 1) * (slices + 1);

        for( i = 0; i < slices; i++)
        {
            sphere->indices[ indexI++] = row_one + i;
            sphere->indices[ indexI++] = row_two + i + 1;
            sphere->indices[ indexI++] = row_two + i;

            sphere->indices[ indexI++] = row_one + i;
            sphere->indices[ indexI++] = row_one + i + 1;
            sphere->indices[ indexI++] = row_two + i + 1;
        }
    }

    sphere->vertices_count = indexV;
    sphere->indices_count = indexI;
}

//
//CreateDisk
//
void CreateDisk( Geometry *disk, float innerRadius, float outerRadius, int stack, int sector)
{
    //variables declaration
    float x, y, z;
    float u, v;
    int i, j;
    int indexV, indexI;

    int vertex_count;
    
    float radius, dr;

    //code
    if(innerRadius <= 0.0f)
        innerRadius = 0.5f;
    
    if(outerRadius <= 0.0f)
        outerRadius = 1.0f;
        
    
    stack = (stack <= 0 ? 5 : stack);
    sector = (sector <= 0 ? 5 : sector);

    dr = (outerRadius - innerRadius) / stack;

    //allocate  memory
    vertex_count = (sector+1) * (stack+1);

    disk->positions = (float *) calloc( 1, 3 * vertex_count * sizeof(float));
    disk->normals   = (float *) calloc( 1, 3 * vertex_count * sizeof(float));
    disk->textures  = (float *) calloc( 1, 2 * vertex_count * sizeof(float));
    disk->tangent   = NULL;

    disk->indices = (unsigned int *) calloc( 1, 2 * 3 * (sector) * (stack) * sizeof(float));

    //fill array
    indexV = 0;
    indexI = 0;
    
    for( int i = 0; i <= stack; i++)
    {
        radius = innerRadius + i * dr;
        for( int j = 0; j <= sector; j++)
        {
            float theta = 2.0f * M_PI * ((float)j / (float)sector);
            x = radius * cos( theta);
            y = 0.0f;
            z = radius * sin( theta);

            disk->positions[ 3 * indexV + 0] = x;
            disk->positions[ 3 * indexV + 1] = y;
            disk->positions[ 3 * indexV + 2] = z;

            disk->normals[ 3 * indexV + 0] = 0;
            disk->normals[ 3 * indexV + 1] = 1;
            disk->normals[ 3 * indexV + 2] = 0;

            disk->textures[ 2 * indexV + 0] = (float)j / (float)sector;
            disk->textures[ 2 * indexV + 1] = (float)i / (float)stack;


            if( (i < stack) && (j < sector))
            {
                disk->indices[ indexI++] =      i  * (sector + 1) + j;
                disk->indices[ indexI++] = (i + 1) * (sector + 1) + j + 1;
                disk->indices[ indexI++] = (i + 1) * (sector + 1) + j;

                disk->indices[ indexI++] =      i  * (sector + 1) + j;
                disk->indices[ indexI++] =      i  * (sector + 1) + j + 1;
                disk->indices[ indexI++] = (i + 1) * (sector + 1) + j + 1;
            }

            indexV++;
        }
    }

    disk->vertices_count = indexV;
    disk->indices_count = indexI;
}

//
//CalculeTangents()
//
int CalculeTangents( Geometry *geometry)
{
    //Tangent Calculation
    Vector3f v0, v1, v2;
    Vector2f tex0, tex1, tex2;
    Vector3f tangent;

    Vector3f edge1, edge2;
    float deltaU1, deltaU2, deltaV1, deltaV2;

    int index1, index2, index3;

    //code
    if( (geometry->positions == NULL) ||
        (geometry->normals == NULL)   ||
        (geometry->textures == NULL)  ||
        (geometry->vertices_count == 0) ||
        (geometry->indices_count == 0)
    )
        return(-1);
        
    
    geometry->tangent   = (float *) calloc( 1, 3 * geometry->vertices_count * sizeof(float));
    
    for (int i = 0; i < geometry->indices_count; i += 3)
    {
        index1 = geometry->indices[i + 0];
        index2 = geometry->indices[i + 1];
        index3 = geometry->indices[i + 2];


        v0.x = geometry->positions[3 * index1 + 0];
        v0.y = geometry->positions[3 * index1 + 1];
        v0.z = geometry->positions[3 * index1 + 2];

        v1.x = geometry->positions[3 * index2 + 0];
        v1.y = geometry->positions[3 * index2 + 1];
        v1.z = geometry->positions[3 * index2 + 2];

        v2.x = geometry->positions[3 * index3 + 0];
        v2.y = geometry->positions[3 * index3 + 1];
        v2.z = geometry->positions[3 * index3 + 2];

        
        tex0.x = geometry->textures[2 * index1 + 0];
        tex0.y = geometry->textures[2 * index1 + 1];

        tex1.x = geometry->textures[2 * index2 + 0];
        tex1.y = geometry->textures[2 * index2 + 1];

        tex2.x = geometry->textures[2 * index3 + 0];
        tex2.y = geometry->textures[2 * index3 + 1];


        edge1.x = v1.x - v0.x;
        edge1.y = v1.y - v0.y;
        edge1.z = v1.z - v0.z;

        edge2.x = v2.x - v0.x;
        edge2.y = v2.y - v0.y;
        edge2.z = v2.z - v0.z;

        deltaU1 = tex1.x - tex0.x;
        deltaV1 = tex1.y - tex0.y;

        deltaU2 = tex2.x - tex0.x;
        deltaV2 = tex2.y - tex0.y;

        float f = 1.0f / (deltaU1 * deltaV2 - deltaU2 * deltaV1);

        tangent.x = f * (deltaV2 * edge1.x - deltaV1 * edge2.x);
        tangent.y = f * (deltaV2 * edge1.y - deltaV1 * edge2.y);
        tangent.z = f * (deltaV2 * edge1.z - deltaV1 * edge2.z);


        geometry->tangent[3 * index1 + 0] += tangent.x;
        geometry->tangent[3 * index1 + 1] += tangent.y;
        geometry->tangent[3 * index1 + 2] += tangent.z;

        geometry->tangent[3 * index2 + 0] += tangent.x;
        geometry->tangent[3 * index2 + 1] += tangent.y;
        geometry->tangent[3 * index2 + 2] += tangent.z;

        geometry->tangent[3 * index3 + 0] += tangent.x;
        geometry->tangent[3 * index3 + 1] += tangent.y;
        geometry->tangent[3 * index3 + 2] += tangent.z;
    }

    Vector3f vec;
    for (int i = 0; i < geometry->vertices_count; i++)
    {
        vec.x = geometry->tangent[3 * i + 0];
        vec.y = geometry->tangent[3 * i + 1];
        vec.z = geometry->tangent[3 * i + 2];

        vec.normalize();

        geometry->tangent[3 * i + 0] = vec.x;
        geometry->tangent[3 * i + 1] = vec.y;
        geometry->tangent[3 * i + 2] = vec.z;
    }
    
    return( 0);
}


//
//DeleteGeometry()
//
void DeleteGeometry( Geometry *geometry)
{
    if( geometry->tangent)
    {
        free(geometry->tangent);
        geometry->tangent = NULL;
    }

    if( geometry->indices)
    {
        free(geometry->indices);
        geometry->indices = NULL;
    }

    if( geometry->textures)
    {
        free(geometry->textures);
        geometry->textures = NULL;
    }

    if( geometry->normals)
    {
        free(geometry->normals);
        geometry->normals = NULL;
    }

    if( geometry->positions)
    {
        free(geometry->positions);
        geometry->positions = NULL;
    }

    geometry->vertices_count = 0;
    geometry->indices_count = 0;
}

