#include<SOIL/SOIL.h>
#include<GL/gl.h>
#include<GL/glu.h>
#include<GL/glut.h>
#include<math.h>
#include<vector>
#include<iostream>
#include "game.h"
using namespace tinyobj;
using namespace std;
/*
 * Loads object in the file "filename" into video memory
 * Allocates a display list ID for the object
 */
LoadedObject::LoadedObject(const char* filename) {
	string err;
	if(!LoadObj(&attributes, &shapes, &materials, &err, filename, 0, false)){
		printf("Oh no!  We got error %s\n", err.c_str());
		return;
	}
	dl = glGenLists(1);
	glNewList(dl, GL_COMPILE);

	for(auto &s : shapes){
		// Add the shape to the display list
		cout << "Loading shape:  " << s.name << endl;
		
		// In case we do something different for each shape
		// But each face can be different too!  So this will fail for some files
		switch(s.mesh.num_face_vertices[0]){
			case 3:
				glBegin(GL_TRIANGLES);
				break;
			case 4:
				glBegin(GL_QUADS);
				break;
			default:
				printf("Unsupported face:  %d sides\n", s.mesh.num_face_vertices);
				return;
		}

		for(auto &i : s.mesh.indices){
			// TODO:  Deal with materials
			// Add the indicated vertex, normal, and texture coordinate
			glVertex3f(attributes.vertices[i.vertex_index*3],
							attributes.vertices[i.vertex_index*3+1],
							attributes.vertices[i.vertex_index*3+2]);
			
			if(i.normal_index != -1) // Then actually load it
				glNormal3f(attributes.normals[i.normal_index*3],
								attributes.normals[i.normal_index*3+2],
								attributes.normals[i.normal_index*3+1]);
			if(i.texcoord_index != -1) {
				glTexCoord2f(attributes.texcoords[i.texcoord_index*2], 
						attributes.texcoords[i.texcoord_index*2+1]);
			}

		}
		glEnd();
	}
	glEndList();
}

LoadedObject::LoadedObject(const char* filename, const char* texturefile) :
	LoadedObject(filename) {
	if(!texturefile)
		return;
	texname = load_texture(texturefile);
}

void LoadedObject::draw(){
	if(texname) {
	   glEnable(GL_TEXTURE_2D);
	   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   	glBindTexture(GL_TEXTURE_2D, texname);	
	}
	glCallList(dl);
	if(texname) {
		glDisable(GL_TEXTURE_2D);
	}
}

SimpleGameObject::SimpleGameObject(float ix, float iy, float iz, const char* objfile, const char* texfile)
	: GameObject(ix, iy, iz), model(objfile, texfile) {}

void SimpleGameObject::draw(){
	glPushMatrix();
	glTranslatef(x, y, z);
	model.draw();
	glPopMatrix();
}
