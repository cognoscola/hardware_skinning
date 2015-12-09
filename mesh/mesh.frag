#version 410

/*
in vec2 pass_textureCoords;
out vec4 out_Color;

uniform sampler2D textureSampler;

void main () {
    out_Color = texture(textureSampler, pass_textureCoords);
}*/

in vec3 normal;
in vec2 st;
in vec3 colour;
out vec4 frag_colour;

 void main(){
    frag_colour = vec4(colour,1.0);
 }