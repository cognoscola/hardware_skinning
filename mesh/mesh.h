//
// Created by alvaregd on 06/12/15.
//

/**
 * Mesh
 *
 * The object imports mesh information from a .dae file including its
 * Bones and animation keyframes and renders it to eh scene
 */


#ifndef WATER_REFLECTION_MESH_H
#define WATER_REFLECTION_MESH_H

#include <GL/gl.h>
#include <utils/math_utils/maths_funcs.h>
#include <camera/camera.h>
#include <assimp/scene.h>

#define TERRAIN_TEXTURE "/home/alvaregd/Documents/Games/hardware_skinning/assets/ao_colour.png"
#define MESH_FILE "/home/alvaregd/Documents/Games/hardware_skinning/assets/seagull.dae"

#define MESH_VERTEX "/home/alvaregd/Documents/Games/hardware_skinning/mesh/mesh.vert"
#define MESH_FRAGMENT "/home/alvaregd/Documents/Games/hardware_skinning/mesh/mesh.frag"

#define BONE_VERTEX "/home/alvaregd/Documents/Games/hardware_skinning/mesh/bone.vert"
#define BONE_FRAGMENT "/home/alvaregd/Documents/Games/hardware_skinning/mesh/bone.frag"

/* max bones allowed in a mesh */
#define MAX_BONES 32

struct SkeletonNode{

    vec3* posKeys;        //describes object's position at every key frame
    versor*rotKeys;       //describes object's orientation at every key frame
    vec3* scaleKeys;      //describes object's scale at every key frame
    double* posKeyTimes;  //key frame times for the position key frames
    double* rotKeyTimes;  //key frame times for the rotation key frames
    double* scaKeyTimes;  //key frame times for the scale key frames
    int numPosKeys;
    int numRotKeys;
    int numScaKeys;

    SkeletonNode *children[MAX_BONES];
    char name[64];
    int numChildren;
    int boneIndex;
};

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
    GLint location_bone_model_mat;

    mat4 modelMatrix;

    int vertexCount;
    int boneCount;
    mat4* monkey_bone_offset_matrices;
    mat4* monkey_bone_animation_mats;
    //rotation
    int bone_matrices_location[MAX_BONES];

    mat4 ear_mat;
    float theta = 0.0f;
    float rot_speed = 50.0f; // 50 radians per second

    SkeletonNode* nodes;
    mat4 g_local_anims[MAX_BONES];
    float y = 0.0; // position of head

    //animation stuff
    double animationDuration;
};


/** initialize the specified mesh object, for now all settings are hardcoded in the function
 * This function will take care of all that is necessary to start using this animal object.
 * call animalRender after calling this function
 * in - animal object
 * in - projection matrix */
void meshInit(Mesh* mesh, GLfloat* proj_mat);

/** loads .dae file into memory
 * in filename: name of the file containing the mesh information
 * in vao:  id of vertex array object to use with opengl
 * out point_count: number of vertices found in them mesh
 * out bone_offset_mats: address to store bone offset information
 * out boneCount: number of bones found
 * out rootNode: address of parent node
 * out animDuration: total animation duration time
 * returns true if function executed successfully */
bool meshLoadMeshFile(
        const char *fileName, GLuint *vao, int *point_count, mat4* bone_offset_mats,
        int* boneCount, SkeletonNode** rootNode, double* animDuration);

/** get the mesh texture into memory from file
 * out mesh: load texture image for the mesh into memory*/
void meshLoadTexture(Mesh* mesh);

/** load a shader program for the specified animal object
 *  for now the file location is hardcoded **/
void meshLoadShaderProgram(Mesh * mesh);

/** get the shader uniforms **/
void meshGetUniforms(Mesh* mesh);

/** render the animal
 * in animal: the animal object to render
 * in camera: the view camera*/
void meshRender(Mesh* mesh, Camera* camera, GLfloat planeHeight);

/** deletes the animal object and frees up memory*/
void meshCleanUp(Mesh *mesh);

/** import the bones and its children
 * in assimpNode: address to assimp object containing node information
 * out skeletonNode: address to rootnode containing where we will update node information
 * in bonecount: number of expected bones to import
 * in boneNames: name of bone*/
bool meshImportSkeletonNode(aiNode* assimpNode, SkeletonNode **skeletonNode, int boneCount, char boneNames[][64]);

/** calculate the animal objects transformation for the next frame
 * in node: the node to perform calculations on
 * in animTime: the time progression of the animation
 * in parentMat: the current node's parent's transformation matrix
 * in boneOffsetMats: the bone's offset matrix
 * out boneAnimationmats: address to point to the updated transformation matrix of the current node**/
void meshSkeletonAnimate(Mesh* mesh,
                         SkeletonNode* node,
                         double animTime,
                         mat4 parentMat,
                         mat4* boneOffsetMats,
                         mat4* boneAnimationMats);

/** finds a particular bone in the skeleton by traversing through the bone hierarchy
 * in root: the root node that is at the beginning of the traversal process
 * in nodeName: name of the node to retrieve
 * returns the address of the node we are looking for */
SkeletonNode* findNodeInSkeleton(SkeletonNode* root, const char* nodeName);
#endif //WATER_REFLECTION_MESH_H
