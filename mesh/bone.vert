#version 410

in vec3 vp;
uniform mat4 proj, view;
uniform mat4 model;

void main () {
	gl_PointSize = 20.0;
	gl_Position = proj * view * model * vec4 (vp, 1.0);
}