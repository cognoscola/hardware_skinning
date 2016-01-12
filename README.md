# Skinning

This my first attempt at animating a mesh.
Check out the scene here ( https://www.instagram.com/p/BAXu1dbNgbF/ )

Much of the code is based off of Anton's Opengl Tutorial Book (http://antongerdelan.net/opengl/#ebook)

First, load the mesh and animation information from a .dae file into memory using Assimp mesh importing tool.
The book explains the process, but essentially assimp does most of the work of reading the file for you and all
that is required is for you to sort through the assimp classes to get the information that you need. In this case,
we fetch vertex coordinates, vertex normals, texture coordinates, bone offset matrices, animation transformations
and their keyframes and bone weight values.

In this case, I made it so that bone heads show up as black dots. However, I do not animate their movement here. In the
shader, I've made it so that the skin colour is a function of the boneId, so that one can get an idea of which
parts of the mesh correspond to which bone.

In order to animate the meshes, we get the transformation information at each keyframe and linearly interpolate
between them. This information (a set of translation and scaling vectors as well as a rotation versor) is converted
into a transformation matrix and fed to the shader for each bone.



