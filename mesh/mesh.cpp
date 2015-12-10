//
// Created by alvaregd on 06/12/15.
//

#include <stdio.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <GL/glew.h>
#include <utils/io/texture.h>
#include <utils/io/shader_loader.h>
#include "mesh.h"



void meshInit(Mesh* mesh, GLfloat* proj_mat){

    mesh->monkey_bone_offset_matrix =(mat4*)malloc(sizeof(mat4) * MAX_BONES );

    assert(meshLoadMeshFile(MESH_FILE,&mesh->vao,&mesh->vertexCount,mesh->monkey_bone_offset_matrix,&mesh->boneCount));
    printf("Monkey Bone Count %i\n", mesh->boneCount);

//    meshLoadTexture(mesh);
    meshLoadShaderProgram(mesh);
    glUseProgram(mesh->shader);
    meshGetUniforms(mesh);
//    glUniform4f(mesh->location_clip_plane, 0.0f, -1.0f, 0.0f, 1.0f);
    glUniformMatrix4fv(mesh->location_projection_mat , 1, GL_FALSE, proj_mat);
    mat4 s = rotate_x_deg(identity_mat4(), -90);
//    mat4 s = scale(identity_mat4(), vec3(10,10,10));
    mesh->modelMatrix = s;
    glUniformMatrix4fv(mesh->location_model_mat , 1, GL_FALSE, mesh->modelMatrix.m);

//////////////////////////

/////////visualizing the bones
    float bone_positions[3*256];
    int c = 0;
    for (int i = 0; i < mesh->boneCount; i++) {
        print(mesh->monkey_bone_offset_matrix[i]);

        // get the x y z translation elements from the last column in the array
        bone_positions[c++] = -mesh->monkey_bone_offset_matrix[i].m[12];
        bone_positions[c++] = -mesh->monkey_bone_offset_matrix[i].m[13];
        bone_positions[c++] = -mesh->monkey_bone_offset_matrix[i].m[14];

        printf("Position[%i]");

    }
    glGenVertexArrays (1, &mesh->boneVao);
    glBindVertexArray (mesh->boneVao);
    GLuint bones_vbo;
    glGenBuffers (1, &bones_vbo);
    glBindBuffer (GL_ARRAY_BUFFER, bones_vbo);
    glBufferData (
            GL_ARRAY_BUFFER,
            3 * mesh->boneCount* sizeof (float),
            bone_positions,
            GL_STATIC_DRAW
    );
    glVertexAttribPointer (0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray (0);

////////////////////////////////////////////

    ///////////////// LOAD BONE SHADER
    mesh->boneShader= create_programme_from_files(BONE_VERTEX, BONE_FRAGMENT);

//////////////////////assign the matrices

    //reset bone matrice

    char name[64];
    for (int j = 0; j < MAX_BONES; ++j) {
        sprintf(name, "bone_matrices[%i]", j);
//        printf("bone_matrices[%i]: %s", j,name);
        mesh->bone_matrices_location[j] = glGetUniformLocation(mesh->shader, name);
        glUniformMatrix4fv(mesh->bone_matrices_location[j], 1, GL_FALSE, identity_mat4().m);
    }

    glUseProgram(mesh->boneShader);
    mesh->location_bone_proj_mat = glGetUniformLocation(mesh->boneShader, "proj");
    mesh->location_bone_view_mat = glGetUniformLocation(mesh->boneShader, "view");
    glUniformMatrix4fv(mesh->location_bone_proj_mat, 1, GL_FALSE, proj_mat);

/////////////////////////////////////////

}

mat4 convert_assimp_matrix (aiMatrix4x4 m) {
    return mat4 (
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            m.a4, m.b4, m.c4, m.d4
    );
}

bool meshLoadMeshFile(const char *fileName, GLuint *vao, int *point_count, mat4* bone_offset_mats, int* boneCount){

    const aiScene *scene = aiImportFile(fileName, aiProcess_Triangulate);
    if (!scene) {
        printf("Error Reading mesh\n");
        fprintf(stderr, "ERROR: reading mesh %s\n", fileName);
        return false;
    }

    printf(" %i animations\n", scene->mNumAnimations);
    printf(" %i Cameras\n", scene->mNumCameras);
    printf(" %i lights\n", scene->mNumLights);
    printf(" %i Materials\n", scene->mNumMaterials);
    printf(" %i Meshes\n", scene->mNumMeshes);
    printf(" %i textures\n", scene->mNumTextures);

    /*get the first mesh */
    const aiMesh* mesh = scene->mMeshes[0];
    printf("  %i vertices in mesh[0]\n", mesh->mNumVertices);

    /**pass back the mesh count */
    *point_count  = mesh->mNumVertices;

    /* generate */
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);

    /** make the data contiguous */
    GLfloat * points = NULL;
    GLfloat * normals = NULL;
    GLfloat * texcoords = NULL;
    GLint * bone_ids = NULL;

    if (mesh->HasPositions()) {
        points = (GLfloat *) malloc(*point_count * 3 * sizeof(GLfloat));
        for (int i = 0; i < *point_count; ++i) {
            const aiVector3D *vp = &(mesh->mVertices[i]);
            points[i * 3 + 0] = (GLfloat)vp->x;
            points[i * 3 + 1] = (GLfloat)vp->y;
            points[i * 3 + 2] = (GLfloat)vp->z;
        }
    }

    if (mesh->HasNormals()) {
        normals = (GLfloat *) malloc(*point_count * 3 * sizeof(GLfloat));
        for (int i = 0; i < *point_count; ++i) {
            const aiVector3D *vp = &(mesh->mNormals[i]);
            normals[i * 3 + 0] = (GLfloat)vp->x;
            normals[i * 3 + 1] = (GLfloat)vp->y;
            normals[i * 3 + 2] = (GLfloat)vp->z;

        }
    }

    if (mesh->HasTextureCoords(0)) {
        texcoords = (GLfloat *) malloc(*point_count * 2 * sizeof(GLfloat));
        for (int i = 0; i < *point_count; ++i) {
            const aiVector3D *vp = &(mesh->mTextureCoords[0][i]);
            texcoords[i * 2 + 0] = (GLfloat)vp->x;
            texcoords[i * 2 + 1] = (GLfloat)vp->y;

        }
    }

    //bones
    if(mesh->HasBones()){
        *boneCount = (int)mesh->mNumBones;

        //max 256 bones, with each bone name < 64 bytes
        char bone_names[256][64];

        bone_ids = (int *) malloc(*point_count * sizeof(int));

        for (int b_i = 0; b_i < *boneCount; b_i++) {
            const aiBone *bone = mesh->mBones[b_i];

            //get the bone name
            strcpy(bone_names[b_i], bone->mName.data);
            printf("bone_names[%i]=%s\n", b_i, bone_names[b_i]);

            //get the position
            bone_offset_mats[b_i] = convert_assimp_matrix(bone->mOffsetMatrix);

            //get the bone weights
            int num_weights = (int) bone->mNumWeights;
            for (int w_i = 0; w_i < num_weights; w_i++) {
                aiVertexWeight weight = bone->mWeights[w_i];
                int vertex_id = (int) weight.mVertexId;
                //ignore weight of less than 0.5 factor
                if (weight.mWeight >= 0.5) {
                    bone_ids[vertex_id]= b_i;
                }
            }
        }

    }

    /** make vbos*/
    if (mesh->HasPositions()) {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * *point_count * sizeof(GLfloat), points, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(0);
        free(points);
    }

    if (mesh->HasNormals()) {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * *point_count * sizeof(GLfloat), normals, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(1);
        free(normals);
    }

    if (mesh->HasTextureCoords(0)) {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 2 * *point_count * sizeof(GLfloat), texcoords, GL_STATIC_DRAW);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(2);
        free(texcoords);
    }

    if (mesh->HasTangentsAndBitangents()) {

    }

    if (mesh->HasBones()) {
        GLuint vbo;
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, *point_count * sizeof(GLint), bone_ids, GL_STATIC_DRAW);
        glVertexAttribIPointer(3, 1, GL_INT, 0, NULL);
        glEnableVertexAttribArray(3);
        free(bone_ids);
    }

    aiReleaseImport(scene);
    printf("Mesh loaded\n");
    return true;
}


void meshLoadTexture(Mesh* mesh){

    GLuint texID;
    glGenTextures(1, &texID);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);

    unsigned char* image_data;
    int x ,y;
    loadImageFile(TERRAIN_TEXTURE, true, &image_data, &x,&y);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    free(image_data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    mesh->texture = texID;
}

void meshLoadShaderProgram(Mesh * mesh){
    mesh->shader = create_programme_from_files(MESH_VERTEX, MESH_FRAGMENT);
}

void meshGetUniforms(Mesh* mesh){
    mesh->location_model_mat   = glGetUniformLocation(mesh->shader, "modelMatrix");
    mesh->location_view_mat   = glGetUniformLocation(mesh->shader, "viewMatrix");
    mesh->location_projection_mat  = glGetUniformLocation(mesh->shader, "projectionMatrix");
//    mesh->location_clip_plane      = glGetUniformLocation(mesh->shader, "plane");
}

void meshRender(Mesh* mesh, Camera* camera, GLfloat planeHeight){

    //render the mesh
    glUseProgram(mesh->shader);
//    glUniform4f(mesh->location_clip_plane, 0.0f, 1.0f, 0.0f, planeHeight);
    glUniformMatrix4fv(mesh->location_view_mat, 1, GL_FALSE, camera->viewMatrix.m);
    glUniformMatrix4fv(mesh->location_model_mat, 1, GL_FALSE, mesh->modelMatrix.m);
    glBindVertexArray(mesh->vao);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mesh->texture);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertexCount);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);

    //render the bone
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(mesh->boneShader);
    glUniformMatrix4fv(mesh->location_bone_view_mat, 1, GL_FALSE, camera->viewMatrix.m);
    glBindVertexArray(mesh->boneVao);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_POINT, 0, mesh->boneCount);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
}

void meshCleanUp(Mesh *mesh){
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);
}

void moveEarsForward(Mesh *mesh, float elapsed_seconds){

    mesh->theta += mesh->rot_speed * elapsed_seconds;

    glUseProgram(mesh->shader);
    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrix[0]) *
                    rotate_z_deg(identity_mat4(), mesh->theta) *
                    mesh->monkey_bone_offset_matrix[0];
    glUniformMatrix4fv(mesh->bone_matrices_location[0], 1, GL_FALSE, mesh->ear_mat.m);

    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrix[1]) *
                    rotate_z_deg(identity_mat4(), -mesh->theta) *
                    mesh->monkey_bone_offset_matrix[1];
    glUniformMatrix4fv(mesh->bone_matrices_location[1], 1, GL_FALSE, mesh->ear_mat.m);

}

void moveEarsBackward(Mesh *mesh, float elapsed_seconds){

    mesh->theta -= mesh->rot_speed * elapsed_seconds;

    glUseProgram(mesh->shader);
    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrix[0]) *
                    rotate_z_deg(identity_mat4(), mesh->theta) *
                    mesh->monkey_bone_offset_matrix[0];
    glUniformMatrix4fv(mesh->bone_matrices_location[0], 1, GL_FALSE, mesh->ear_mat.m);

    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrix[1]) *
                    rotate_z_deg(identity_mat4(), -mesh->theta) *
                    mesh->monkey_bone_offset_matrix[1];
    glUniformMatrix4fv(mesh->bone_matrices_location[1], 1, GL_FALSE, mesh->ear_mat.m);

}
