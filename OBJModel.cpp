#include "OBJModel.h"

/**
    SPECIALLY CHANGE FOR "Toren1BD.obj" File
**/


struct VERTEX
{
    bool   is_set = false;
    float  position[3];
    int    textureIndex;
    int    normalIndex;
    int    index;
    int    nextIndex = -1; //work like linked-list but by index not by pointer
};

#define BUFFER_SIZE 512

void LoadOBJModel( const char *filename, Model_Data *model_data, int vertexIndex, int textureIndex, int normalIndex)
{
    //variable declarations
    std::vector<struct VERTEX*> verticesArray;
    struct VERTEX *temp;

    std::vector<GLfloat> textures;
    std::vector<GLfloat> normals;
    std::vector<GLuint> indices;

    char line[BUFFER_SIZE];

    char *firstToken, *token;

    FILE *fp = NULL;

    GLfloat v[3];
    GLfloat vt[2];
    GLfloat vn[3];

    GLfloat *vertex_array = NULL;
    GLfloat *texture_array = NULL;
    GLfloat *normal_array = NULL;
    
    std::vector<GLuint> verticesIndices;
    std::vector<GLuint> texturesIndices;
    std::vector<GLuint> normalsIndices;

    GLint vertex_count, texture_count, normal_count, indices_count;

    int vi, ti, ni;
    char *faceVertex[3];

    //code
    fp = fopen(filename, "r");
    if(fp == NULL)
    {
        printf("Error \"%s\"", filename);
        return;
    }


    while( fgets( line, BUFFER_SIZE, fp) != NULL)
    {
        firstToken = strtok( line, " ");

        if( strcmp(firstToken, "v") == 0)
        {
            v[0] = atof( strtok( NULL, " "));
            v[1] = atof( strtok( NULL, " "));
            v[2] = atof( strtok( NULL, " "));

            temp = (struct VERTEX *) malloc( sizeof(struct VERTEX));

            temp->is_set = false;
            temp->nextIndex = -1;
            temp->textureIndex   = -1;
            temp->normalIndex    = -1;
            temp->position[0]    = v[0];
            temp->position[1]    = v[1];
            temp->position[2]    = v[2];
            temp->index = verticesArray.size();

            verticesArray.push_back( temp);
        }
        else if(strcmp(firstToken, "vt") == 0)
        {
            vt[0] = atof( strtok( NULL, " "));
            vt[1] = atof( strtok( NULL, " "));

            textures.push_back( vt[0]);
            textures.push_back( vt[1]);
        }
        else if(strcmp(firstToken, "vn") == 0)
        {
            vn[0] = atof( strtok( NULL, " "));
            vn[1] = atof( strtok( NULL, " "));
            vn[2] = atof( strtok( NULL, " "));

            normals.push_back( vn[0]);
            normals.push_back( vn[1]);
            normals.push_back( vn[2]);
        }
        else if(strcmp(firstToken, "f") == 0)
        {
            faceVertex[0] = strtok( NULL, " "); // v1/t1/n1
            faceVertex[1] = strtok( NULL, " "); // v2/t2/n2
            faceVertex[2] = strtok( NULL, " "); // v3/t3/n3
            //printf("%s %s %s", faceVertex[0], faceVertex[1], faceVertex[2]);

            
            for( int k = 0; k < 3; k++)
            {
                firstToken = strtok( faceVertex[k], "/"); // v
                vi = atoi(firstToken) - 1;
                ti = atoi( strtok( NULL, "/")) - 1;
                ni = atoi( strtok( NULL, "/")) - 1;
                
                verticesIndices.push_back( vi);
                texturesIndices.push_back( ti);
                normalsIndices.push_back( ni);
                /*printf(
                    "\t%d/%d/%d ",
                    verticesIndices[ verticesIndices.size() - 1],
                    texturesIndices[ texturesIndices.size() - 1],
                    normalsIndices[ normalsIndices.size() - 1]
                );*/
            }
            //printf("\n\n");
        }
    }

//     for(int i = 0; i < verticesArray.size()/3; i++)
//     {
//         printf("%d/%d/%d ", verticesArray[3*i + 0]->index, verticesArray[3*i + 0]->textureIndex, verticesArray[3*i + 0]->normalIndex);
//         printf("%d/%d/%d ", verticesArray[3*i + 1]->index, verticesArray[3*i + 1]->textureIndex, verticesArray[3*i + 1]->normalIndex);
//         printf("%d/%d/%d ", verticesArray[3*i + 2]->index, verticesArray[3*i + 2]->textureIndex, verticesArray[3*i + 2]->normalIndex);
//         printf("\n");
//     }

    // for(int i = 0; i < indices.size()/3; i++)
    // {
    //     printf("%d %d %d\n", indices[3*i + 0], indices[3*i + 1], indices[3*i + 2]);
    // }

    // printf("verticesArray size: %lu\n", verticesArray.size());
    // printf("indicesArray size : %lu\n", indices.size());
    // printf("indicesArray size/3 : %lu\n", indices.size()/3);

    

/*    vert_array = (GLfloat*) malloc( vertices.size() * sizeof(GLfloat));
    for(int i = 0; i < vertices.size(); i++)
    {
        vert_array[i] = vertices[i];
    }

    index_array = (GLuint*) malloc( indices.size() * sizeof(GLuint));
    for(int i = 0; i < indices.size(); i++)
    {
        index_array[i] = indices[i];
    }
*/
    for( int i = 0; i < verticesIndices.size(); i++)
    {
        //break;
        
        vi = verticesIndices[i];
        ti = texturesIndices[i];
        ni = normalsIndices[i];

        temp = verticesArray[vi];
        if(temp->is_set == false)
        {
            temp->textureIndex = ti;
            temp->normalIndex = ni;
            temp->is_set = true;

            indices.push_back( vi);
        }
        else
        {
            if(temp->textureIndex == ti && temp->normalIndex == ni) //data already in indices
            {
                indices.push_back( vi);
            }
            else
            {
                bool found = false;

                while( temp->nextIndex != -1)
                {
                    temp = verticesArray[ temp->nextIndex];
                    if(temp->textureIndex == ti && temp->normalIndex == ni)
                    {
                        found = true;
                        indices.push_back( temp->index);
                        break;
                    }
                }

                if(found == false)
                {
                    struct VERTEX *temp_2 = (struct VERTEX *) malloc( sizeof(struct VERTEX));
                    temp_2->is_set = true;
                    temp_2->nextIndex = -1;
                    temp_2->textureIndex   = ti;
                    temp_2->normalIndex    = ni;
                    temp_2->position[0]    = temp->position[0];
                    temp_2->position[1]    = temp->position[1];
                    temp_2->position[2]    = temp->position[2];
                    temp_2->index = verticesArray.size();

                    temp->nextIndex = temp_2->index;   //store index where duplicate vertices present
                    verticesArray.push_back(temp_2);
                    indices.push_back( temp_2->index);
                }
            }
        }
    }

    printf("\n");
    printf("verticesIndices: %lu\n", verticesIndices.size());
    printf("texturesIndices: %lu\n", texturesIndices.size());
    printf("normalsIndices: %lu\n", normalsIndices.size());

    //allocate memory for arrays
    vertex_array = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 3 );
    texture_array   = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 2 );
    normal_array   = (GLfloat *) malloc( sizeof(GLfloat) * verticesArray.size() * 3 );


    //convert to array
    for( int i = 0; i < verticesArray.size(); i++)
    {
        
        ti = verticesArray[i]->textureIndex;
        ni = verticesArray[i]->normalIndex;

        *(vertex_array + 3*i + 0) = verticesArray[i]->position[0];
        *(vertex_array + 3*i + 1) = verticesArray[i]->position[1];
        *(vertex_array + 3*i + 2) = verticesArray[i]->position[2];

        *(texture_array + 2*i + 0) = textures[2*ti + 0];
        *(texture_array + 2*i + 1) = textures[2*ti + 1];

        *(normal_array + 3*i + 0) = normals[3*ni + 0];
        *(normal_array + 3*i + 1) = normals[3*ni + 1];
        *(normal_array + 3*i + 2) = normals[3*ni + 2];
    }
    
    struct VEC3
    {
        float x, y, z;
    };
    
    //Calculate Normals
    for( int i = 0 ; i < indices.size()/3; i++)
    {
        VEC3 p0, p1, p2;
        float normal[3];
        int index[3];
        
        index[0] = indices[ 3 * i + 0];
        index[1] = indices[ 3 * i + 1];
        index[2] = indices[ 3 * i + 2];
        
        p0.x = *(vertex_array + 3*index[0] + 0);
        p0.y = *(vertex_array + 3*index[0] + 1);
        p0.z = *(vertex_array + 3*index[0] + 2);
        
        p1.x = *(vertex_array + 3*index[1] + 0);
        p1.y = *(vertex_array + 3*index[1] + 1);
        p1.z = *(vertex_array + 3*index[1] + 2);
        
        p2.x = *(vertex_array + 3*index[2] + 0);
        p2.y = *(vertex_array + 3*index[2] + 1);
        p2.z = *(vertex_array + 3*index[2] + 2);
        
        VEC3 v1, v2;
        
        v1.x = p1.x - p0.x;
        v1.y = p1.y - p0.y;
        v1.z = p1.z - p0.z;
        
        v2.x = p2.x - p0.x;
        v2.y = p2.y - p0.y;
        v2.z = p2.z - p0.z;
        
        VEC3 v1Xv2;
        v1Xv2.x =    v1.y * v2.z - v1.z * v2.y;
        v1Xv2.y = - (v1.x * v2.z - v1.z * v2.x);
        v1Xv2.z =    v1.x * v2.y - v1.y * v2.x;
        
        float length = (float) sqrt( v1Xv2.x*v1Xv2.x + v1Xv2.y*v1Xv2.y + v1Xv2.z*v1Xv2.z);
        
        normal[0] = v1Xv2.x/length;
        normal[1] = v1Xv2.y/length;
        normal[2] = v1Xv2.z/length;
        
        
        *(normal_array + 3*index[0] + 0) = normal[0];
        *(normal_array + 3*index[0] + 1) = normal[1];
        *(normal_array + 3*index[0] + 2) = normal[2];
        
        *(normal_array + 3*index[1] + 0) = normal[0];
        *(normal_array + 3*index[1] + 1) = normal[1];
        *(normal_array + 3*index[1] + 2) = normal[2];
        
        *(normal_array + 3*index[2] + 0) = normal[0];
        *(normal_array + 3*index[2] + 1) = normal[1];
        *(normal_array + 3*index[2] + 2) = normal[2];
    }

    vertex_count = verticesArray.size();
    texture_count = verticesArray.size();
    normal_count = verticesArray.size();
    indices_count = indices.size();

    printf("Vertex Array Size: %d\n", vertex_count*3);
    printf("Number Of vertex : %d\n", vertex_count);

    printf("Texture Array Size: %d\n", texture_count*2);
    printf("Number Of texture : %d\n", texture_count);

    printf("Normals Array Size: %d\n", normal_count*3);
    printf("Number Of normal : %d\n", normal_count);

    printf("Indices : %d\n", indices_count);
    printf("Faces : %d\n", indices_count/3);

/*
struct Model_Data
{
    GLuint vao;
    GLuint vbo_position;
    GLuint vbo_texture;
    GLuint vbo_normals;
    GLuint vbo_elements;
    GLint  numberOfElements;
};
*/

    glGenVertexArrays( 1, &model_data->vao);
    glBindVertexArray( model_data->vao);

        //position
        glGenBuffers( 1, &model_data->vbo_position);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_position);

            glBufferData( GL_ARRAY_BUFFER, 3 * vertex_count * sizeof(GLfloat), vertex_array, GL_STATIC_DRAW);
            glVertexAttribPointer( vertexIndex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( vertexIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

            //texture
        glGenBuffers( 1, &model_data->vbo_texture);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_texture);

            glBufferData( GL_ARRAY_BUFFER, 2 * texture_count * sizeof(GLfloat), texture_array, GL_STATIC_DRAW);
            glVertexAttribPointer( textureIndex, 2, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( textureIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

            //normals
        glGenBuffers( 1, &model_data->vbo_normals);
        glBindBuffer( GL_ARRAY_BUFFER, model_data->vbo_normals);

            glBufferData( GL_ARRAY_BUFFER, 3 * normal_count * sizeof(GLfloat), normal_array, GL_STATIC_DRAW);
            glVertexAttribPointer( normalIndex, 3, GL_FLOAT, GL_FALSE, 0, NULL);
            glEnableVertexAttribArray( normalIndex);

        glBindBuffer( GL_ARRAY_BUFFER, 0);

        //Elements
        glGenBuffers( 1, &model_data->vbo_elements);
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, model_data->vbo_elements);

            glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices_count * sizeof(GLuint), &indices[0], GL_STATIC_DRAW);

        //glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0);

    glBindVertexArray( 0);


    //save indices count
    model_data->numberOfElements = indices_count;
    model_data->numberOfVertices = vertex_count;


    //clean-up
    fclose( fp);
    fp = NULL;

    //clear vectors
    textures.clear();
    normals.clear();
    indices.clear();
    
    verticesIndices.clear();
    texturesIndices.clear();
    normalsIndices.clear();

    //free memory
    for(int i = 0; i < verticesArray.size(); i++)
    {
            //clear memory allocated to each member of vector
        free(verticesArray[i]);
        verticesArray[i] = NULL;
    }

    verticesArray.clear();

    //free memroy of arrays
    free( vertex_array);
    vertex_array = NULL;

    free( texture_array);
    texture_array = NULL;

    free( normal_array);
    normal_array = NULL;
}


void UnloadOBJModel( Model_Data* model)
{
    //code
    if( model->vbo_elements)
    {
        glDeleteBuffers( 1, &model->vbo_elements);
        model->vbo_elements = 0;
    }
    
    if( model->vbo_normals)
    {
        glDeleteBuffers( 1, &model->vbo_normals);
        model->vbo_normals = 0;
    }
    
    if( model->vbo_texture)
    {
        glDeleteBuffers( 1, &model->vbo_texture);
        model->vbo_texture = 0;
    }
    
    if( model->vbo_position)
    {
        glDeleteBuffers( 1, &model->vbo_position);
        model->vbo_position = 0;
    }
    
    if( model->vao)
    {
        glDeleteVertexArrays( 1, &model->vao);
        model->vao = 0;
    }
    
    model->numberOfElements = 0;
    model->numberOfVertices = 0;
}



/*

    ( x1,y1)   (x2,y2) 
        --------
        |     /|
        |    / |
        |   /  |
        |  /   |
        | /    |
        --------
    (x3, y3)  (x4,y4)


    
    x1,y1  x2,y2  x3,y3
    x2,y2  x3,y3  x4,y4


  
    12 * 4 = 48
    12 * 4 = 48
    (6 * 3) * 4 = 96

    192

--------------------------------------------

    x1,y1   x2,y2  x3,y3  x4,y4
    8*4       = 32
    8*4       = 32
    (4*3) * 4 = 48
                24

                136

    1, 2, 3
    2, 3, 4

    8*4 + 6*4 = 32 + 24 = 56
*/


