#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef union {
    struct{float x,y,z,r,g,b,u,v;};
    float _xyzrgbuv[8];
} Vertex;

typedef union{
    struct {float x,y;};
    float _xy[2];
} vec2;

typedef union{
    struct {float x,y,z;};
    float _xyz[3];
} vec3;

typedef struct {
    char        m_objName[128],
                m_matName[128];
    Vertex      *m_vertices;
    uint32_t    *m_indices;
    uint32_t    m_vSize,
                m_iSize;
} Mesh;

typedef struct {
    Mesh        *m_mesh;
    uint32_t    m_size;
} Model;

const char obj[] = 
"# Blender 3.6.1\n"
"# www.blender.org\n"
"mtllib untitled.mtl\n"
"o Cube\n"
"v 1.000000 1.000000 -1.000000\n"
"v 1.000000 -1.000000 -1.000000\n"
"v 1.000000 1.000000 1.000000\n"
"v 1.000000 -1.000000 1.000000\n"
"v -1.000000 1.000000 -1.000000\n"
"v -1.000000 -1.000000 -1.000000\n"
"v -1.000000 1.000000 1.000000\n"
"v -1.000000 -1.000000 1.000000\n"
"vn -0.0000 1.0000 -0.0000\n"
"vn -0.0000 -0.0000 1.0000\n"
"vn -1.0000 -0.0000 -0.0000\n"
"vn -0.0000 -1.0000 -0.0000\n"
"vn 1.0000 -0.0000 -0.0000\n"
"vn -0.0000 -0.0000 -1.0000\n"
"vt 0.625000 0.500000\n"
"vt 0.875000 0.500000\n"
"vt 0.875000 0.750000\n"
"vt 0.625000 0.750000\n"
"vt 0.375000 0.750000\n"
"vt 0.625000 1.000000\n"
"vt 0.375000 1.000000\n"
"vt 0.375000 0.000000\n"
"vt 0.625000 0.000000\n"
"vt 0.625000 0.250000\n"
"vt 0.375000 0.250000\n"
"vt 0.125000 0.500000\n"
"vt 0.375000 0.500000\n"
"vt 0.125000 0.750000\n"
"s 0\n"
"usemtl Material\n"
"f 1/1/1 5/2/1 7/3/1 3/4/1\n"
"f 4/5/2 3/4/2 7/6/2 8/7/2\n"
"f 8/8/3 7/9/3 5/10/3 6/11/3\n"
"f 6/12/4 2/13/4 4/5/4 8/14/4\n"
"f 2/13/5 1/1/5 3/4/5 4/5/5\n"
"f 6/11/6 5/10/6 1/1/6 2/13/6\n"
;

typedef uint8_t b8;
#define SC_TRUE     1
#define SC_FALSE    0


// this right here is the raunchiest ass code i've ever made
// i fucking hate every line of this affront to our lord and savior above
// this code is a fucking war crime

b8 string_to_vec3(  const char *str, 
                    size_t size,
                    vec3* out)
{
    char *data_ptr; 
    uint32_t length = 0;

    out->x = strtof(str, &data_ptr);
    length = data_ptr - str;
    if (size <= length)
        return SC_FALSE;

    out->y = strtof(data_ptr, &data_ptr);
    length = data_ptr - str;
    if (size <= length)
        return SC_FALSE;

    out->z = strtof(data_ptr, &data_ptr);
    length = data_ptr - str;
    if (size <= length)
        return SC_FALSE;

    return SC_TRUE;
}

b8 string_to_vec2(  const char *str, 
                    size_t size,
                    vec2* out)
{
    char *data_ptr; 
    uint32_t length = 0;

    out->x = strtof(str, &data_ptr);
    length = data_ptr - str;
    if (size <= length)
        return SC_FALSE;

    out->y = strtof(data_ptr, &data_ptr);
    length = data_ptr - str;
    if (size <= length)
        return SC_FALSE;

    return SC_TRUE;
}

b8 parse_triplet(   const char *str, 
                    size_t size, 
                    uint32_t *v, 
                    uint32_t *vt, 
                    uint32_t *vn)
{
    uint32_t    *properties[] = {v,vt,vn};    
    uint32_t    property    = 0,
                str_index   = 0;

    char *data_ptr;
    if (size < 1 || str[0] == '/')
        return SC_FALSE;

    *properties[property++] = (int)strtol(str, &data_ptr, 10);
    str_index = data_ptr - str;
    if (str_index >= size)
        return SC_TRUE;

    if (str[str_index + 1]=='/')
    {
        property++;
        str_index++;
        data_ptr++;
    }
    else
    {
        *properties[property++] = (int)strtol(data_ptr+1, &data_ptr, 10);
        str_index = data_ptr - str;
        if (str_index >= size)
            return SC_TRUE;
    }

    if (str[str_index + 1]=='/')
        return SC_FALSE;

    *properties[property++] = (int)strtol(data_ptr+1, &data_ptr, 10);
    str_index = data_ptr - str;
    if (str_index >= size)
        return SC_TRUE;

    return SC_TRUE;
}

b8 parse_triplet_cluster(   const char *data, 
                            size_t      size,
                            uint32_t    *vid,
                            uint32_t    *vtid,
                            uint32_t    *vnid,
                            uint32_t    *vi_size)
{
    uint32_t    line_length = 0,
                i           = 0,
                start_idx   = i,
                viSize      = 0;

    // If the line starts with 'f', increment index twice ahead of time
    if(data[i] == 'f')
        i++;

    i++;
    
    // Get the first vertex
    while (data[i+line_length] != ' ') 
    {
        line_length++; 
        if(i+line_length>size) 
            return SC_FALSE;
    }
    parse_triplet(&data[i], line_length, &vid[viSize], &vtid[viSize], &vnid[viSize]);
    //printf("%d %d %d\n", vid[viSize], vtid[viSize], vnid[viSize]);
    i += line_length+1;
    line_length = 0;
    viSize++;

    // Get the second vertex
    while (data[i+line_length] != ' ') 
    {
        line_length++; 
        if(i+line_length>size) 
            return SC_FALSE;
    }
    parse_triplet(&data[i], line_length, &vid[viSize], &vtid[viSize], &vnid[viSize]);
    //printf("%d %d %d\n", vid[viSize], vtid[viSize], vnid[viSize]);
    i += line_length+1;
    line_length = 0;
    viSize++;

    // Get the third vertex
    while ( data[i+line_length] != ' '  && 
            data[i+line_length] != '\n' && 
            data[i+line_length] != '\0') 
    {
        line_length++; 
        if(i+line_length-1>size) 
            return SC_FALSE;
    }
    parse_triplet(&data[i], line_length, &vid[viSize], &vtid[viSize], &vnid[viSize]);
    //printf("%d %d %d\n", vid[viSize], vtid[viSize], vnid[viSize]);
    i += line_length+1;
    line_length = 0;
    viSize++;

    if (i-start_idx >= size)
        return SC_TRUE;

    // If there's a fourth vertex, get that too
    while ( data[i+line_length] != ' '  && 
            data[i+line_length] != '\n' && 
            data[i+line_length] != '\0') 
    {
        line_length++; 
        if(i+line_length-1>size) 
            return SC_FALSE;
    }
    parse_triplet(&data[i], line_length, &vid[viSize], &vtid[viSize], &vnid[viSize]);
    //printf("%d %d %d\n", vid[viSize], vtid[viSize], vnid[viSize]);
    i += line_length+1;
    line_length = 0;
    viSize++;

    // Set the vertex indices size
    *vi_size    = viSize;
    
    // Reset 
    viSize = 0;
    return SC_TRUE;
}

void load_obj_file(const char *data, size_t size)
{
    vec3        *v,
                *vn;
    vec2        *vt;

    Model       m = {};
    Mesh        *activeMesh;

    uint32_t    block_size  = 2048, // Block length
                vSize       = 0,    // Position, normal, UV list sizes
                vnSize      = 0,
                vtSize      = 0,
                vCapacity   = block_size,   // List capacities
                vnCapacity  = block_size,
                vtCapacity  = block_size,
                vtxCapacity = block_size,
                idxCapacity = block_size,
                line_length = 0;

    uint32_t    vid[4],         // Face index lists
                vtid[4],
                vnid[4],
                viSize          = 0,
                idxSegmentSize  = 0;

    if (size < 1)
        return;

    v   = (vec3*)malloc(block_size*sizeof(vec3));
    vn  = (vec3*)malloc(block_size*sizeof(vec3));
    vt  = (vec2*)malloc(block_size*sizeof(vec2));

    // Initialize the model
    m.m_size = 0;
    
    /*
        For every object
            Get material
                Get vertices
                Get texture coords
                Get normals
            Use material
            Get faces
    */
    for (int i = 0; i < size; i++)
    {
        line_length = 0;
        switch (data[i])
        {
            case 'm':
                break;
            case 'o':
                // Set the next active mesh
                if (m.m_mesh != NULL)
                    m.m_mesh = realloc(m.m_mesh, (++m.m_size)*sizeof(Mesh));
                else
                    m.m_mesh = malloc(++m.m_size*sizeof(Mesh));

                // Set pointer to active mesh
                activeMesh = &m.m_mesh[m.m_size-1];

                // Initialize attributes here
                activeMesh->m_iSize = 0;
                activeMesh->m_vSize = 0;
                activeMesh->m_vertices    = (Vertex*)malloc(block_size*sizeof(Vertex));
                activeMesh->m_indices     = (uint32_t*)malloc(block_size*sizeof(uint32_t));

                // Go to next line
                while (data[i+line_length] != '\n'){line_length++;}
                
                // Copy the object name
                i++;
                memcpy(activeMesh->m_objName, data+i, line_length);
                activeMesh->m_objName[line_length] = '\0';
                break;
            case 'v':
                switch (data[++i])
                {
                    case ' ':   // Vertex
                        if ((vSize+1)>vCapacity)
                        {
                            vCapacity += block_size;
                            v = (vec3*)realloc(v, vCapacity*sizeof(vec3));
                        }

                        while (data[i+line_length]!='\n') {line_length++;}
                        string_to_vec3(&data[i], line_length, &v[vSize++]);
                        break;
                    case 't':   // Texture
                        i++;
                        if ((vtSize+1)>vtCapacity)
                        {
                            vtCapacity += block_size;
                            vt = (vec2*)realloc(vt, vtCapacity*sizeof(vec2));
                        }

                        while (data[i+line_length]!='\n') {line_length++;}
                        string_to_vec2(&data[i], line_length, &vt[vtSize++]);
                        break;
                    case 'n':   // Normal
                        i++;
                        if ((vnSize+1)>vnCapacity)
                        {
                            vnCapacity += block_size;
                            vn = (vec3*)realloc(vn, vnCapacity*sizeof(vec3));
                        }

                        while (data[i+line_length]!='\n') {line_length++;}
                        string_to_vec3(&data[i], line_length, &vn[vnSize++]);
                        break;
                }
                break;
            case 'f':   // Build vertices
                while (data[i+line_length]!='\n' && data[i+line_length]!='\0') {line_length++;}
                parse_triplet_cluster(  &data[i], 
                                        line_length, 
                                        vid, 
                                        vtid, 
                                        vnid, 
                                        &viSize);

                // THIS IS MY SAD EXCUSE OF AN ERROR HANDLER
                if (viSize < 0)
                    break;
                
                // Resize the vertex array as needed
                if (activeMesh->m_vSize+viSize > vtxCapacity)
                {
                    vtxCapacity += block_size;
                    activeMesh->m_vertices = (Vertex*)realloc(
                        activeMesh->m_vertices, 
                        vtxCapacity*sizeof(Vertex)
                    );
                }

                // Assume thate there are only 4 or 3 faces (I AM GOING TO LOOK LIKE A CLOWN IF IT 
                // TURNS OUT YOU CAN HAVE MORE THAN 4 OR LESS THAN 2 FACES AND IT WOULD STILL BE A
                // VALID OBJ FILE) 
                idxSegmentSize = ((viSize==4)?6:3);
                if ((activeMesh->m_iSize+idxSegmentSize)>vtxCapacity)
                {
                    idxCapacity += block_size;
                    activeMesh->m_indices = (uint32_t*)realloc(
                        activeMesh->m_indices, 
                        idxCapacity*sizeof(uint32_t)
                    );
                }

                // Copy the vertices
                for (int i = 0; i < viSize; i++)
                {
                    // Position
                    activeMesh->m_vertices[activeMesh->m_vSize].x = v[vid[i]-1].x;
                    activeMesh->m_vertices[activeMesh->m_vSize].y = v[vid[i]-1].y;
                    activeMesh->m_vertices[activeMesh->m_vSize].z = v[vid[i]-1].z;
                    
                    // Normal
                    activeMesh->m_vertices[activeMesh->m_vSize].r = vn[vnid[i]-1].x;
                    activeMesh->m_vertices[activeMesh->m_vSize].g = vn[vnid[i]-1].y;
                    activeMesh->m_vertices[activeMesh->m_vSize].b = vn[vnid[i]-1].z;
                    
                    // UV
                    activeMesh->m_vertices[activeMesh->m_vSize].u = vt[vtid[i]-1].x;
                    activeMesh->m_vertices[activeMesh->m_vSize].v = vt[vtid[i]-1].y;
                    
                    activeMesh->m_vSize++;
                }
                
                // Copy the indices
                for (int i = 0; i < idxSegmentSize/3; i++)
                {
                    activeMesh->m_indices[activeMesh->m_iSize+0] = vid[0+i]-1;
                    activeMesh->m_indices[activeMesh->m_iSize+1] = vid[1+i]-1;
                    activeMesh->m_indices[activeMesh->m_iSize+2] = vid[2+i]-1;

                    activeMesh->m_iSize += 3;
                }

                break;
            default: while(data[i] != '\n') {i++;}
        }
    }

    for (int mi = 0; mi < m.m_size; mi++)
    {
        printf("\nObject: %s\n", m.m_mesh[mi].m_objName);
        for (int i = 0; i < m.m_mesh[mi].m_vSize; i++)
        {
            printf("Position: %+f, %+f, %+f  Normal: %+f, %+f, %+f  UV: %f, %f\n",
                activeMesh->m_vertices[i].x,
                activeMesh->m_vertices[i].y,
                activeMesh->m_vertices[i].z,
                activeMesh->m_vertices[i].r,
                activeMesh->m_vertices[i].g,
                activeMesh->m_vertices[i].b,
                activeMesh->m_vertices[i].u,
                activeMesh->m_vertices[i].v);
        }

        for (int i = 0; i < m.m_mesh[mi].m_iSize; i++)
        {
            if (i % 3 == 0)
                printf("\n");
            printf("%d, ", activeMesh->m_indices[i]);
        }
        printf("\n");
    }

    // Free temporary list of vertex components
    free(v);
    free(vn);
    free(vt);
    
    // Free the active meshes (this should be done outside this function)
    for (int i = 0; i < m.m_size; i++)
    {
        free(m.m_mesh[i].m_vertices);
        free(m.m_mesh[i].m_indices);
    }
    free(m.m_mesh);
}

int main()
{
    vec3 a = {0};
    uint32_t    v[4]    = {0}, 
                vt[4]   = {0}, 
                vn[4]   = {0},
                vSize   = 0,
                index   = 0;

    //const char face[] = "f 6/11/6 5/10/6 1/1/6 2/13/6";
    //parse_triplet_cluster(  face, 
    //                        sizeof(face), 
    //                        v, 
    //                        vt, 
    //                        vn, 
    //                        &vSize);
    //
    //for (int i = 0; i < vSize; i++)
    //    printf("f %d/%d/%d\n", v[i], vt[i], vn[i]);

    load_obj_file(obj, sizeof(obj));
    //const char  vector[] = "-1.0000 1.00000 2.00000",
    //            triplet[] = "2/13/4";
    //string_to_vec3(vector, sizeof(vector), &a);
    //
    //printf("%f, %f, %f\n", a.x, a.y, a.z);
    //parse_triplet(triplet, sizeof(triplet)-1, &v[0], &vt[0], &vn[0]);
    //printf("%d, %d, %d\n", v[0],vt[0],vn[0]);

    return 0;
}