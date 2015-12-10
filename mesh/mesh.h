//
// Created by alvaregd on 06/12/15.
//

#ifndef WATER_REFLECTION_MESH_H
#define WATER_REFLECTION_MESH_H

#include <GL/gl.h>
#include <utils/math_utils/maths_funcs.h>
#include <camera/camera.h>

#define TERRAIN_TEXTURE "/home/alvaregd/Documents/Games/hardware_skinning/assets/ao_colour.png"
#define MESH_FILE "/home/alvaregd/Documents/Games/hardware_skinning/assets/monkey.dae"

#define MESH_VERTEX "/home/alvaregd/Documents/Games/hardware_skinning/mesh/mesh.vert"
#define MESH_FRAGMENT "/home/alvaregd/Documents/Games/hardware_skinning/mesh/mesh.frag"

#define BONE_VERTEX "/home/alvaregd/Documents/Games/hardware_skinning/mesh/bone.vert"
#define BONE_FRAGMENT "/home/alvaregd/Documents/Games/hardware_skinning/mesh/bone.frag"

/* max bones allowed in a mesh */
#define MAX_BONES 32

struct Mesh{

    GLuint shader;
    GLuint texture;
    GLuint vao;
    GLuint vbo;

    GLuint boneShader;
    GLuint boneVao;
    GLuint boneVbo;

    GLint location_projection_mat;
    GLint location_view_mat;
    GLint location_model_mat;

    GLint location_bone_view_mat;
    GLint location_bone_proj_mat;

    mat4 modelMatrix;

    int vertexCount;
    int boneCount;
    mat4* monkey_bone_offset_matrix;

    //rotation
    int bone_matrices_location[MAX_BONES];
    mat4 ear_mat;
    float theta = 0.0f;
    float rot_speed = 50.0f; // 50 radians per second

};

void meshInit(Mesh* mesh, GLfloat* proj_mat);
bool meshLoadMeshFile(const char *fileName, GLuint *vao, int *point_count, mat4 *bone_offset_mats, int* boneCount);
void meshLoadTexture(Mesh* mesh);
void meshLoadShaderProgram(Mesh * mesh);
void meshGetUniforms(Mesh* mesh);
void meshRender(Mesh* mesh, Camera* camera, GLfloat planeHeight);
void meshCleanUp(Mesh *mesh);
void moveEarsForward(Mesh *mesh, float elapsed_seconds);
void moveEarsBackward(Mesh *mesh, float elapsed_seconds);
#endif //WATER_REFLECTION_MESH_H
