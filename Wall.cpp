#include<GL/gl.h>
#include<GL/glu.h>
#include<GL/glut.h>
#include<math.h>
#include<string.h>
#include "game.h"

/*
 * Corners:  
 * 	c1-c4 is lower level, starting with +x, +z and moving clockwise
 *		c5-c8 is the same, for the upper level
 *		All faces are specified clockwise from upper left, when viewing straight on
 */

#define c1 (0.5, -0.5, 0.5)
#define c2 (-0.5, -0.5, 0.5)
#define c3 (-0.5, -0.5, -0.5) 
#define c4 (0.5, -0.5, -0.5) 
#define c5 (0.5, 0.5, 0.5) 
#define c6 (-0.5, 0.5, 0.5) 
#define c7 (-0.5, 0.5, -0.5) 
#define c8 (0.5, 0.5, -0.5) 

WallType::WallType(const char* texfile, float* im, float* is){
	if(texfile)
		texname = load_texture(texfile);
	memcpy(material, im, sizeof(float) * 4);
	memcpy(spec, is, sizeof(float) * 4);
	dl = glGenLists(1);
	glNewList(dl, GL_COMPILE);
	glBegin(GL_QUADS);
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
	// Face 1:  +x
	glNormal3f(1, 0, 0);
	glTexCoord2f(0, 1);
	glVertex3f c5;
	glTexCoord2f(1, 1);
	glVertex3f c8;
	glTexCoord2f(1, 0);
	glVertex3f c4;
	glTexCoord2f(0, 0);
	glVertex3f c1;
	// Face 2:  -x
	glNormal3f(-1, 0, 0);
	glTexCoord2f(0, 1);
	glVertex3f c7;
	glTexCoord2f(1, 1);
	glVertex3f c6;
	glTexCoord2f(1, 0);
	glVertex3f c2;
	glTexCoord2f(0, 0);
	glVertex3f c3;
	// Face 3:  +z (from starting camera)
	glNormal3f(0, 0, 1);
	glTexCoord2f(0, 1);
	glVertex3f c2;
	glTexCoord2f(1, 1);
	glVertex3f c1;
	glTexCoord2f(1, 0);
	glVertex3f c5;
	glTexCoord2f(0, 0);
	glVertex3f c6;
	// Face 4:  -z
	glNormal3f(0, 0, -1);
	glTexCoord2f(0, 1);
	glVertex3f c4;
	glTexCoord2f(1, 1);
	glVertex3f c3;
	glTexCoord2f(1, 0);
	glVertex3f c7;
	glTexCoord2f(0, 0);
	glVertex3f c8;
	// Face 5:  +y (from above, up = 0,0,1)
	glNormal3f(0, 1, 0);
	glTexCoord2f(0, 0);
	glVertex3f c5;
	glTexCoord2f(0, 1);
	glVertex3f c6;
	glTexCoord2f(1, 1);
	glVertex3f c7;
	glTexCoord2f(1, 0);
	glVertex3f c8;
	// Face 6:  -y (from below, up = 0,0,1)
	glNormal3f(0, -1, 0);
	glTexCoord2f(0, 0);
	glVertex3f c2;
	glTexCoord2f(0, 1);
	glVertex3f c1;
	glTexCoord2f(1, 1);
	glVertex3f c4;
	glTexCoord2f(1, 0);
	glVertex3f c3;
	glEnd();
	glEndList();
}

void WallType::draw(float x, float y, float z, float size_x, float size_y, float size_z){
	glPushMatrix();
	glTranslatef(x, y, z);
	glScalef(size_x, size_y, size_z);
	if(texname) {
	   glEnable(GL_TEXTURE_2D);
	   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   	glBindTexture(GL_TEXTURE_2D, texname);	
	}
	glCallList(dl);
	if(texname) 
		glDisable(GL_TEXTURE_2D);
	glPopMatrix();
}

void WallTile::draw(){
	wt->draw(x, y, z, size_x, size_y, size_z);
}

bool WallTile::overlap(float ox, float oy, float oz, float margin){
	float dx = fabs(ox - x);
	float dy = fabs(oy - y);
	float dz = fabs(oz - z);
	if(dx < size_x/2 + margin)
		if(dy < size_y/2 + margin)
			if(dz < size_z/2 + margin)
				return true;
	return false;
}

bool WallTile::over(float ox, float oz) {
	float dx = fabs(ox - x);
	float dz = fabs(oz - z);
	if(dx < size_x/2)
		if(dz < size_z/2)
				return true;
	return false;
}

char WallTile::blockDirection(float ox, float oy, float oz, float nx, float ny, float nz, float margin){
	if(overlap(ox, oy, oz, margin))
		return 0;
	if(overlap(nx, oy, oz, margin))
		return 'x';
	if(overlap(ox, ny, oz, margin))
		return 'y';
	if(overlap(ox, oy, nz, margin))
		return 'z';
	return 0;
}
