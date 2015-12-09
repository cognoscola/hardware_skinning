#version 410

layout(location = 0)in vec3 positions;
layout(location = 1)in vec3 vertex_normal;
layout(location = 2)in vec2 textureCoords;
layout(location = 3)in int bone_id;

uniform mat4 viewMatrix;
uniform mat4 modelMatrix;
uniform mat4 projectionMatrix;

uniform mat4 bone_matrices[64];

out vec3 normal;
out vec2 st;
out vec3 colour;

void main(void){

    colour = vec3(0.0,0.0,0.0);
    if(bone_id == 0){
        colour.r = 1.0;
    }else if (bone_id == 1){
        colour.g= 1.0;
    }else if(bone_id == 2){
        colour.b = 1.0;
    }

    st = textureCoords;
    normal = vertex_normal;
    gl_Position = projectionMatrix  *  viewMatrix * modelMatrix * bone_matrices[bone_id] * vec4 (positions, 1.0);


}