
struct Geometry
{
    int vertices_count;
    int indices_count;

    float *positions;
    float *textures;
    float *normals;
	float *tangent;
    unsigned int *indices;
};

extern "C"
{
    //function declarations
    void CreateSphere( float radius, int slices, int stacks,  Geometry *sphere);
    void CreateDisk( Geometry *disk, float innerRadius, float outerRadius, int stack, int sector);

        //return -1 if geometry position, normal, texture are not calculated.
        //return 0 on success
    int CalculeTangents( Geometry *geometry);

    void DeleteGeometry( Geometry *geometry);
}

