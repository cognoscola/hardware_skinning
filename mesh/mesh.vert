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
        colour.r = 0.0;
        colour.g = 1.0;
        colour.b = 0.0;
    }
    else if(bone_id == 0){
                  colour.r = 1.0;
                  colour.g = 0.0;
                  colour.b = 0.0;
    }
    else if(bone_id == 1){
                      colour.r = 1.0;
                      colour.g = 0.5;
                      colour.b = 1.0;
        }
    else if(bone_id == 2){
                      colour.r = 0.5;
                      colour.g = 0.5;
                      colour.b = 1.0;
        }
    else if(bone_id == 3){
                      colour.r = 0.0;
                      colour.g = 0.0;
                      colour.b = 1.0;
        }
    else if(bone_id == 4){
                      colour.r = 0.0;
                      colour.g = 0.5;
                      colour.b = 1.0;
        }
    else if(bone_id == 5){
                      colour.r = 0.0;
                      colour.g = 0.7;
                      colour.b = 1.0;
        }
    else if(bone_id == 6){
                      colour.r = 0.0;
                      colour.g = 1.0;
                      colour.b = 0.0;
        }
    else if(bone_id == 7){
                      colour.r = 0.5;
                      colour.g = 1.0;
                      colour.b = 0.0;
        }
    else if(bone_id == 8){
                       colour.r = 1.0;
                       colour.g = 1.0;
                       colour.b = 0.0;
    }
    else if(bone_id == 9){
         colour.r = 1.0;
         colour.g = 0.5;
         colour.b = 0.0;
    }
    else if(bone_id == 10){
          colour.r = 1.0;
          colour.g = 1.0;
          colour.b = 1.0;
    }
    else if(bone_id == 11){
         colour.r = 1.0;
         colour.g = 0.5;
         colour.b = 0.0;
    }
     else if(bone_id == 12){
             colour.r = 0.4;
             colour.g = 0.4;
             colour.b = 1.0;
        }
     else if(bone_id == 13){
             colour.r = 0.5;
             colour.g = 0.0;
             colour.b = 1.0;
        }
     else if(bone_id == 14){
             colour.r = 1.0;
             colour.g = 0.0;
             colour.b = 1.0;
        }

      else if(bone_id == 15){
              colour.r = 1.0;
              colour.g = 0.2;
              colour.b = 0.2;
         }
      else if(bone_id == 16){
              colour.r = 0.0;
              colour.g = 0.6;
              colour.b = 0.6;
         }
      else if(bone_id == 17){
              colour.r = 0.4;
              colour.g = 0.4;
              colour.b = 0.4;
         }

     else if(bone_id == 18){
             colour.r = 0.8;
             colour.g = 0.8;
             colour.b = 1.0;
     }

     else if(bone_id == 19){
             colour.r = 1.0;
             colour.g = 0.0;
             colour.b = 0.4;
        }
     else if(bone_id == 20){
             colour.r = 1.0;
             colour.g = 0.0;
             colour.b = 0.8;
        }
     else if(bone_id == 21){
             colour.r = 0.0;
             colour.g = 1.0;
             colour.b = 0.6;
        }

    st = textureCoords;
    normal = vertex_normal;
    gl_Position = projectionMatrix  *  viewMatrix * modelMatrix * bone_matrices[bone_id]* vec4 (positions, 1.0);

//bone_matrices[bone_id]

}