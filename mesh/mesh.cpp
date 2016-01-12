// Created by alvaregd on 06/12/15.
#include <stdio.h>
#include <assimp/cimport.h>

#include <assimp/postprocess.h>
#include <GL/glew.h>
#include <utils/io/texture.h>
#include <utils/io/shader_loader.h>
#include "mesh.h"

void meshInit(Mesh* mesh, GLfloat* proj_mat){

    mesh->monkey_bone_offset_matrices =(mat4*)malloc(sizeof(mat4) * MAX_BONES );
    mesh->monkey_bone_animation_mats = (mat4*)malloc(sizeof(mat4) * MAX_BONES );
    for (int i = 0; i < MAX_BONES; i++) {
        mesh->monkey_bone_offset_matrices[i] = identity_mat4();
    }
    assert(meshLoadMeshFile(
            MESH_FILE,
            &mesh->vao,
            &mesh->vertexCount,
            mesh->monkey_bone_offset_matrices,
            &mesh->boneCount,
            &mesh->nodes,
            &mesh->animationDuration
   ));

    printf("Bone Count %i\n", mesh->boneCount);

//    meshLoadTexture(mesh);
    meshLoadShaderProgram(mesh);
    glUseProgram(mesh->shader);
    meshGetUniforms(mesh);
    glUniformMatrix4fv(mesh->location_projection_mat , 1, GL_FALSE, proj_mat);
    mat4 rotX = rotate_x_deg(identity_mat4(), -90.0f);
    mat4 rotY = rotate_y_deg(identity_mat4(), -90.0f);
    mat4 Trans = translate(identity_mat4(),vec3(0.0f,0.0f,-3.0f));
    mesh->modelMatrix = Trans *rotY*rotX ;
    glUniformMatrix4fv(mesh->location_model_mat , 1, GL_FALSE, mesh->modelMatrix.m);

/////////visualizing the bones
    float bone_positions[3*256];
    int c = 0;
    for (int i = 0; i < mesh->boneCount; i++) {
        print(mesh->monkey_bone_offset_matrices[i]);
        // get the x y z translation elements from the last column in the array
        bone_positions[c++] = -mesh->monkey_bone_offset_matrices[i].m[12];
        bone_positions[c++] = -mesh->monkey_bone_offset_matrices[i].m[13];
        bone_positions[c++] = -mesh->monkey_bone_offset_matrices[i].m[14];
        printf("Position[%i]",i);
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
    for (int j = 0; j < MAX_BONES; j++) {
        sprintf(name, "bone_matrices[%i]", j);
//        printf("bone_matrices[%i]: %s", j,name);
        mesh->bone_matrices_location[j] = glGetUniformLocation(mesh->shader, name);
        glUniformMatrix4fv(mesh->bone_matrices_location[j], 1, GL_FALSE, identity_mat4().m);
    }

    glUseProgram(mesh->boneShader);
    mesh->location_bone_proj_mat = glGetUniformLocation(mesh->boneShader, "proj");
    mesh->location_bone_view_mat = glGetUniformLocation(mesh->boneShader, "view");
    mesh->location_bone_model_mat = glGetUniformLocation(mesh->boneShader, "model");
    glUniformMatrix4fv(mesh->location_bone_proj_mat, 1, GL_FALSE, proj_mat);
    glUniformMatrix4fv(mesh->location_bone_model_mat, 1, GL_FALSE,  mesh->modelMatrix.m);

/////////////////////////////////////////
}

mat4 convert_assimp_matrix (aiMatrix4x4 m) {
    return mat4 (
 /*           1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            m.a4, m.b4, m.c4, m.d4*/

           m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
    );
}

bool meshLoadMeshFile(
        const char *fileName, GLuint *vao, int *point_count, mat4* bone_offset_mats,
        int* boneCount, SkeletonNode** rootNode, double* animDuration){

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
        for (int i = 0; i < *point_count; i++) {
            const aiVector3D *vp = &(mesh->mVertices[i]);
            points[i * 3 + 0] = (GLfloat)vp->x;
            points[i * 3 + 1] = (GLfloat)vp->y;
            points[i * 3 + 2] = (GLfloat)vp->z;
        }
    }

    if (mesh->HasNormals()) {
        normals = (GLfloat *) malloc(*point_count * 3 * sizeof(GLfloat));
        for (int i = 0; i < *point_count; i++) {
            const aiVector3D *vp = &(mesh->mNormals[i]);
            normals[i * 3 + 0] = (GLfloat)vp->x;
            normals[i * 3 + 1] = (GLfloat)vp->y;
            normals[i * 3 + 2] = (GLfloat)vp->z;
        }
    }

    if (mesh->HasTextureCoords(0)) {
        texcoords = (GLfloat *) malloc(*point_count * 2 * sizeof(GLfloat));
        for (int i = 0; i < *point_count; i++) {
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
            printf("\n");
            printf ("[%.2f][%.2f][%.2f][%.2f]\n", bone->mOffsetMatrix.a1, bone->mOffsetMatrix.a2, bone->mOffsetMatrix.a3, bone->mOffsetMatrix.a4);
            printf ("[%.2f][%.2f][%.2f][%.2f]\n", bone->mOffsetMatrix.b1, bone->mOffsetMatrix.b2, bone->mOffsetMatrix.b3, bone->mOffsetMatrix.b4);
            printf ("[%.2f][%.2f][%.2f][%.2f]\n", bone->mOffsetMatrix.c1, bone->mOffsetMatrix.c2, bone->mOffsetMatrix.c3, bone->mOffsetMatrix.c4);
            printf ("[%.2f][%.2f][%.2f][%.2f]\n", bone->mOffsetMatrix.d1, bone->mOffsetMatrix.d2, bone->mOffsetMatrix.d3, bone->mOffsetMatrix.d4);

            bone_offset_mats[b_i] = convert_assimp_matrix(bone->mOffsetMatrix);
//            bone_offset_mats[b_i] = convert_assimp_matrix(bone->mOffsetMatrix);

            //get the bone weights
            int num_weights = (int) bone->mNumWeights;
            for (int w_i = 0; w_i < num_weights; w_i++) {
                aiVertexWeight weight = bone->mWeights[w_i];
                int vertex_id = (int) weight.mVertexId;
                //ignore weight of less than 0.5 factor
                bone_ids[vertex_id]= b_i;
//                if (weight.mWeight >= 0.5) {
//                }
            }
        }

        aiNode* assimpNode = scene->mRootNode;
        if(!meshImportSkeletonNode(assimpNode,rootNode,*boneCount,bone_names)){
            fprintf(stderr, "ERROR: could not import node tree from mesh\n");
        }

        if(scene->mNumAnimations > 0){

            //get the first animations
            aiAnimation* anim = scene->mAnimations[0];
            printf("animation name: %s\n", anim->mName.C_Str());
            printf("animation has: %i node channels\n", anim->mNumChannels);
            printf("animation has: %i mesh channels\n", anim->mNumMeshChannels);
            printf("animation duration %f\n", anim->mDuration);
            printf("ticks per second %f\n", anim->mTicksPerSecond);

            *animDuration = anim->mDuration;
            printf("anim duration is %f\n", anim->mDuration);

            for (int i = 0; i < (int) anim->mNumChannels; i++) {
                aiNodeAnim* chan = anim->mChannels[i];

                //find the matching node in our skeleton
                SkeletonNode* sn = findNodeInSkeleton(*rootNode, chan->mNodeName.C_Str());

                if (!sn) {
                    fprintf(stderr, "WARNING: did not find node named %s in skeleton. Animation Broken!. \n",
                            chan->mNodeName.C_Str());
                    continue;
                }

                sn->numPosKeys = chan->mNumPositionKeys;
                sn->numRotKeys = chan->mNumRotationKeys;
                sn->numScaKeys = chan->mNumScalingKeys;


                sn->posKeys = (vec3 *) malloc(sizeof(vec3) * sn->numPosKeys);
                sn->rotKeys = (versor *) malloc(sizeof(versor) * sn->numRotKeys);
                sn->scaleKeys = (vec3 *) malloc(sizeof(vec3) * sn->numScaKeys);

                sn->posKeyTimes = (double *) malloc(sizeof(double) * sn->numPosKeys);
                sn->rotKeyTimes = (double *) malloc(sizeof(double) * sn->numRotKeys);
                sn->scaKeyTimes = (double *) malloc(sizeof(double) * sn->numScaKeys);

                //add position keys to node
                for (int j = 0; j < sn->numPosKeys; j++) {
                    aiVectorKey key = chan->mPositionKeys[j];
                    sn->posKeys[j].v[0] = key.mValue.x;
                    sn->posKeys[j].v[1] = key.mValue.y; //TODO BLENDER HAS Z as up
                    sn->posKeys[j].v[2] = key.mValue.z;
                    sn->posKeyTimes[j] = key.mTime;
                }

                for (int k = 0; k < sn->numRotKeys; k++) {
                    aiQuatKey key = chan->mRotationKeys[k];
                    sn->rotKeys[k].q[0] = key.mValue.w;
                    sn->rotKeys[k].q[1] = key.mValue.x;
                    sn->rotKeys[k].q[2] = key.mValue.y;
                    sn->rotKeys[k].q[3] = key.mValue.z;
                    sn->rotKeyTimes[k] = key.mTime;
                }

                for (int l = 0; l < sn->numScaKeys; l++) {
                    aiVectorKey key = chan->mScalingKeys[l];
                    sn->scaleKeys[l].v[0] = key.mValue.x;
                    sn->scaleKeys[l].v[1] = key.mValue.y;
                    sn->scaleKeys[l].v[2] = key.mValue.z;
                    sn->scaKeyTimes[l] = key.mTime;
                }
            }
        }else {
            fprintf(stderr, "WARNING: no animations found in mesh file\n");
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
}

void meshRender(Mesh* mesh, Camera* camera, GLfloat planeHeight){

    //render the mesh
    glUseProgram(mesh->shader);
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
    glPointSize(1);
    glDrawArrays(GL_POINTS, 0, mesh->boneCount);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_DEPTH_TEST);
}

void meshCleanUp(Mesh *mesh){
    glDeleteVertexArrays(1, &mesh->vao);
    glDeleteBuffers(1, &mesh->vbo);

//    free(mesh->nodes->posKeys);
//    free(mesh->nodes->rotKeys);
//    free(mesh->nodes->scaleKeys);

//    free(mesh->nodes->posKeyTimes);
//    free(mesh->nodes->rotKeyTimes);
//    free(mesh->nodes->scaKeyTimes);
}

bool meshImportSkeletonNode(aiNode* assimpNode, SkeletonNode** skeletonNode, int boneCount, char boneNames[][64]){

    SkeletonNode *temp = (SkeletonNode *) malloc(sizeof(SkeletonNode));
    strcpy(temp->name, assimpNode->mName.C_Str());
    printf("-node name = %s\n", temp->name);
    temp->numChildren = 0;

    //initialize key frames info
    temp->posKeys = NULL;
    temp->rotKeys = NULL;
    temp->scaleKeys = NULL;
    temp->posKeyTimes = NULL;
    temp->rotKeyTimes = NULL;
    temp->scaKeyTimes = NULL;
    temp->numPosKeys = 0;
    temp->numRotKeys = 0;
    temp->numScaKeys = 0;


    printf("node has %i children\n", (int) assimpNode->mNumChildren);
    temp->boneIndex = -1;
    for (int i = 0; i < MAX_BONES; i++) {
        temp->children[i] = NULL;
    }

    bool hasBone = false;
    for (int j = 0; j < boneCount; j++) {
        if (strcmp(boneNames[j], temp->name) == 0) {
            printf("node uses bone %i\n", j);
            temp->boneIndex = j;
            hasBone = true;
            break;
        }
    }

    if (!hasBone) {
        printf("no bone found for node\n");
    }

    bool hasUsefulChild = false;
    for (int k = 0; k < (int)assimpNode->mNumChildren; k++) {
        if(meshImportSkeletonNode(
                assimpNode->mChildren[k],
                &temp->children[temp->numChildren],
                boneCount,
                boneNames
        )){
            hasUsefulChild = true;
            temp->numChildren++;
        }else {
            printf("useless child culled\n");
        }
    }

    if (hasUsefulChild || hasBone) {
        *skeletonNode = temp;
        return true;
    }

    free(temp);
    temp = NULL;
    return  false;
}


void meshSkeletonAnimate(Mesh* mesh,
                         SkeletonNode* node,
                         double animTime,
                         mat4 parentMat,
                         mat4* boneOffsetMats,
                         mat4* boneAnimationMats) {

    assert(node);

    mat4 ourMat = parentMat;
    mat4 localAnim = identity_mat4();

    //interpolate the position
    mat4 nodeT = identity_mat4();
    if (node->numPosKeys > 0) {
        int prevKeys =0;
        int nextKeys =0;
        for (int i = 0; i < node->numPosKeys - 1; i++) {
            prevKeys = i;
            nextKeys =i +1;
            if (node->posKeyTimes[nextKeys] >= animTime) {
                break;
            }
        }
        float total_t = (float)(node->posKeyTimes[nextKeys] - node->posKeyTimes[prevKeys]);
        float t = (float)((animTime - node->posKeyTimes[prevKeys]) / total_t);
        vec3 vi = node->posKeys[prevKeys];
        vec3 vf = node->posKeys[nextKeys];
        vec3 lerped = vi* (1.0f -t ) + vf* t;
        nodeT = translate(identity_mat4(), lerped);
    }

    //sphere interpolate rotations
    mat4 node_R = identity_mat4();
    if(node->numRotKeys > 0) {
        int prevKeys = 0;
        int nextKeys = 0;
        for (int i = 0; i < node->numRotKeys - 1; i++) {
            prevKeys = i;
            nextKeys = i + 1;
            if (node->rotKeyTimes[nextKeys] >= animTime) {
                break;
            }
        }

        float total_t = (float)(node->rotKeyTimes[nextKeys] - node->rotKeyTimes[prevKeys]);
        float t = (float)((animTime - node->rotKeyTimes[prevKeys]) / total_t);
        versor qi = node->rotKeys[prevKeys];
        versor qf = node->rotKeys[nextKeys];
        versor slerped = slerp(qi, qf, t);
        node_R = quat_to_mat4(slerped);
    }


    mat4 node_S = identity_mat4();
    if (node->numScaKeys > 0) {
        int prevKeys =0;
        int nextKeys =0;
        for (int i = 0; i < node->numScaKeys - 1; i++) {
            prevKeys = i;
            nextKeys =i +1;
            if (node->scaKeyTimes[nextKeys] >= animTime) {
                break;
            }
        }
        float total_t = (float)(node->scaKeyTimes[nextKeys] - node->scaKeyTimes[prevKeys]);
        float t = (float)((animTime - node->scaKeyTimes[prevKeys]) / total_t);
        vec3 si = node->scaleKeys[prevKeys];
        vec3 sf = node->scaleKeys[nextKeys];
        vec3 lerped = si* (1.0f -t ) + sf* t;
        node_S = scale(identity_mat4(), lerped);
    }

    localAnim = nodeT * node_R * node_S;

    //if node has a weighted bone
    int bone_i = node->boneIndex;
    if (bone_i > -1) {
        mat4 boneOffset = boneOffsetMats[bone_i];
//        mat4 invBoneOffset = inverse(boneOffset);
//        localAnim = mesh->g_local_anims[bone_i];
        ourMat = parentMat * localAnim;
//        ourMat = parentMat * invBoneOffset * localAnim * boneOffset;
        boneAnimationMats[bone_i] = parentMat * localAnim * boneOffset;

    }

    for (int i = 0; i < node->numChildren; i++) {
        meshSkeletonAnimate(
                mesh,
                node->children[i],
                animTime,
                ourMat,
                boneOffsetMats,
                boneAnimationMats
        );
    }
}

SkeletonNode* findNodeInSkeleton(SkeletonNode* root, const char* nodeName){
    assert(root);

    if (strcmp(nodeName, root->name) == 0) {
        return root;
    }

    for (int i = 0; i < root->numChildren; i++) {
        SkeletonNode *child = findNodeInSkeleton(root->children[i], nodeName);
        if (child != NULL) {
            return child;
        }
    }
    return NULL;
}



void moveEarsForward(Mesh *mesh, float elapsed_seconds){

    mesh->theta += mesh->rot_speed * elapsed_seconds;

    glUseProgram(mesh->shader);
    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrices[0]) *
                    rotate_z_deg(identity_mat4(), mesh->theta) *
                    mesh->monkey_bone_offset_matrices[0];
    glUniformMatrix4fv(mesh->bone_matrices_location[0], 1, GL_FALSE, mesh->ear_mat.m);
    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrices[1]) *
                    rotate_z_deg(identity_mat4(), -mesh->theta) *
                    mesh->monkey_bone_offset_matrices[1];
    glUniformMatrix4fv(mesh->bone_matrices_location[1], 1, GL_FALSE, mesh->ear_mat.m);
}

void moveEarsBackward(Mesh *mesh, float elapsed_seconds){

    mesh->theta -= mesh->rot_speed * elapsed_seconds;
    glUseProgram(mesh->shader);
    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrices[0]) *
                    rotate_z_deg(identity_mat4(), mesh->theta) *
                    mesh->monkey_bone_offset_matrices[0];
    glUniformMatrix4fv(mesh->bone_matrices_location[0], 1, GL_FALSE, mesh->ear_mat.m);

    mesh->ear_mat = inverse(mesh->monkey_bone_offset_matrices[1]) *
                    rotate_z_deg(identity_mat4(), -mesh->theta) *
                    mesh->monkey_bone_offset_matrices[1];
    glUniformMatrix4fv(mesh->bone_matrices_location[1], 1, GL_FALSE, mesh->ear_mat.m);

}
