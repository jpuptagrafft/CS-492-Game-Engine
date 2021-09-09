#include<GL/gl.h>
#include<GL/glu.h>
#include<GL/glut.h>
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<thread>
#include<vector>
#define MAIN
#include "game.h"
using namespace std;

double findDist(double aX, double aY, double aZ, double bX, double bY, double bZ){
	return sqrt(pow(bX - aX, 2) + pow(bY- aY, 2) + pow(bZ - aZ, 2));
}

int width = 1024, height = 768;
static GLuint texName;
int fps = 0;
int movement_loops = 0;
float etime_average = 0.0;
bool invincible = false;
float next_xpos = 0;
float next_zpos = 0;
bool done = false;
bool top_mode = false;
thread* move_thread = 0;

void shutdown(){
	done = true;
	move_thread->join();
	exit(0);
}

void handle_keys_down();


float* getHeadingCoordinates(float offset){
	float* coordinates = (float*)malloc(3 * sizeof(float)); // Sorry, we've only got 20 minutes
	float radius = cos(elevation);
	coordinates[0] = sin(PI+heading+offset) * radius;
	coordinates[1] = sin(elevation);
	coordinates[2] = sin((3*PI/2) + heading + offset) * radius;
	return coordinates;
}

void fire(double size){
	if(GS.equiped == 0){
		Projectile* newp = new Projectile(xpos, ypos, zpos, heading, elevation, size);
		newp->cycle(50);
		newp->time_remaining = 10000;
		newp->velocity = 0.02;
		objects.push_back(newp);
	}
	if(GS.equiped == 1){
		Projectile* newp = new Projectile(xpos, ypos, zpos, heading, elevation, size);
		newp->cycle(50);
		newp->time_remaining = 50;
		newp->velocity = .1;
		objects.push_back(newp);
	}
	if(GS.equiped == 2){
		ExpandingProjectile* newp = new ExpandingProjectile(xpos, ypos, zpos, heading, elevation, size);
		newp->cycle(50);
		newp->time_remaining = 10000;
		newp->velocity = .02;
		objects.push_back(newp);
	}
}		

void drawTarget(){
   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   glBindTexture(GL_TEXTURE_2D, texName);
	glColor3f(0, .8, .4);
	glutSolidTeapot(.5);
   glDisable(GL_TEXTURE_2D);
}

void drawAnotherTarget(){
   glEnable(GL_TEXTURE_2D);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
   glBindTexture(GL_TEXTURE_2D, texName);
	glColor3f(1, 0, 0);
	glutSolidSphere(.5, 3, 3);
   glDisable(GL_TEXTURE_2D);
}

void drawHUD() {
	glLoadIdentity();
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glColor3f(1, 0.5, 1);
	glOrtho(0, width, 0, height, -1.0f, 1.0f);
	char message[100];
	int height = 100;
	auto printmessage = [&message, &height](){
		glRasterPos2i(10, height);
		for(char* i = message; *i; i++)
			glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *i);
		height -= 20;
	};
	sprintf(message, "HP: %d", GS.hp);
	printmessage();
	sprintf(message, "ARMOR: %d", GS.armor);
	printmessage();
	if(!GS.hasGun){
		sprintf(message, "EQUIPED: NONE");
		printmessage();
	} else {
		switch(GS.equiped){
			case 0:
				sprintf(message, "EQUIPED: CHARGE PISTOL (%i/%i)(%d%)", GS.gunOneLoaded, GS.gunOneAmmo, GS.charge);
				printmessage();
				break;
			case 1:
				sprintf(message, "EQUIPED: FLAMETHROWER (%i)", GS.gunTwoAmmo);
				printmessage();
				break;
			case 2:
				sprintf(message, "EQUIPED: ADHEISIVE ACID ACTUATOR (%i)", GS.gunThreeAmmo);
				printmessage();
				break;
		}
	}
	glLoadIdentity();
	glColor3f(1, 1, 0.5);
	glBegin(GL_LINES);
	glVertex3f(0, -0.1, 0);
	glVertex3f(0, 0.1, 0);
	glVertex3f(.1, 0, 0);
	glVertex3f(-.1, 0, 0);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	
}

void drawScene() {
	count++;
	framecount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	gluPerspective(90, float(width)/height, .1, 1000);
	float* lap = getHeadingCoordinates(0);
	if(top_mode)
		gluLookAt(xpos, ypos + 5, zpos,   xpos, ypos, zpos, 0, 0, -1);
	else
		gluLookAt(xpos, ypos, zpos,   xpos + lap[0], ypos + lap[1], zpos + lap[2], hit, 1-hit, hit);
	free(lap);

	glBegin(GL_QUADS);
	for(int x = -2; x < 11; x++){
		for(int z = -3; z < 23; z++){
			float material_color[] = {.5, .5, .5, 0.5};
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material_color);
			float material_spec_color[] = {.5, .5, .5, .5};
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_spec_color);
			glNormal3f(0, 1, 0);
			glVertex3f(x, -1, z);			
			glVertex3f(x+1, -1, z);			
			glVertex3f(x+1, -1, z+1);			
			glVertex3f(x, -1, z+1);			
		}
	}
	glEnd();

	objects_lock.lock();
	for(auto &o : objects)
		if(!o->dead)
			o->draw();
	for(auto &i : pickups)
		if(!i->dead)
			i->draw();
	for(auto &j : Door)
		if(!j->dead)
			j->draw();
	for(auto &k : fake_objects)
		if(!k->dead)
			k->draw();
	
	objects_lock.unlock();
	
	drawHUD();
	glutSwapBuffers();
}

#define SPEED 0.01

void handle_keys_down(size_t etime){
	float* lap = 0;
	if(kstatus.forward){
		lap = getHeadingCoordinates(0);
		next_xpos = xpos + lap[0] * SPEED * etime;
		next_zpos = zpos + lap[2] * SPEED * etime;
	}
	if(kstatus.backward){
		lap = getHeadingCoordinates(PI);
		next_xpos = xpos + lap[0] * SPEED * etime;
		next_zpos = zpos + lap[2] * SPEED * etime;
	}
	if(kstatus.turnleft){
		heading += (PI/320) * etime;
	}
	if(kstatus.turnright){
		heading -= (PI/320) * etime;
	}
	if(kstatus.stepleft){
		lap = getHeadingCoordinates(PI/2);
		next_xpos = xpos + lap[0] * SPEED * etime;
		next_zpos = zpos + lap[2] * SPEED * etime;
	}
	if(kstatus.stepright){
		lap = getHeadingCoordinates(3*PI/2);
		next_xpos = xpos + lap[0] * SPEED * etime;
		next_zpos = zpos + lap[2] * SPEED * etime;
	}
	if(kstatus.fire){
		fire(.1);
	}
	if(lap) free(lap);
}

void handlekey(char status, unsigned char key, int x, int y){
	if(hit) return;
	switch(key){
		case '1':
			if(GS.gunOne){
				GS.equiped = 0;
				GS.charge = 0;
			}
			break;
		case '2':
			if(GS.gunTwo){
				GS.equiped = 1;
				GS.charge = 0;
			}
			break;
		case '3':
			if(GS.gunThree){
				GS.equiped = 2;
				GS.charge = 0;
			}
			break;
		case 'w': // forward
			kstatus.forward = status;
			break;
		case 's': // backward
			kstatus.backward = status;
			break;
		case 'e':
			for(int t = 0; t < pickups.size(); t++){
				if(findDist(xpos, ypos, zpos, pickups[t]->x, pickups[t]->y, pickups[t]->z) <= 1.5){
					pickups[t]->interact();
					delete pickups[t];
					pickups[t] = pickups[pickups.size() - 1];
					pickups.pop_back();
				}
			}
			break;
		case 'a': // left
			kstatus.stepleft = status;
			break;
		case 'd': // right
			kstatus.stepright = status;
			break;
		case 'x':
			kstatus.fire = status;
			break;
		case 't':
			if(status)
				top_mode = !top_mode;
			break;
		case ' ':
			if(yv < 0.01 && status && GS.jump_delay <= 0)
				yv = .02;
				GS.jump_delay = 20;
			break;
		case 'r':
			switch(GS.equiped){
				case 0:
					if(GS.gunOneAmmo > 0 && GS.gunOneLoaded != GS.gunOneClip){
						GS.gunOneLoaded = GS.gunOneClip;
						GS.gunOneAmmo -= GS.gunOneClip;
					}
					break;
			}
			break;
		 
	}
}

void keyup(unsigned char key, int x, int y){
	handlekey(0, key, x, y);
}

void keypress(unsigned char key, int x, int y){
	switch(key) {
		case 'i':
			invincible = !invincible;
			break;
		case 27:
			shutdown();
		default:
			handlekey(1, key, x, y);
	}
}


#define ELMAX (PI/2 - 0.01)
void mousemove(int x, int y){
	int height = 1024;
	int width = 768;
	int dx = height/2 - x;
	int dy = width/2 - y;
	heading += dx/300.0;
	elevation += dy/300.0;
	if(elevation >= ELMAX) elevation = ELMAX;
	if(elevation <= -ELMAX) elevation = -ELMAX;
	if(dx || dy)
		glutWarpPointer(height/2, width/2);
}

void mousefire(int button, int state, int x, int y){
	if(GS.hasGun){
		printf("Button:  %d  state:  %d\n", button, state);
		printf("X = %f, Y = %f, Z = %f\n", xpos, ypos, zpos);
		switch(GS.equiped){
			case 0:
				if(button == 0){
					if(GS.gunOneLoaded > 0 && GS.delay == 0 && !GS.charging){
						fire(.1);
						GS.gunOneLoaded--;
						GS.delay = GS.gunOneDelay;
					}
				} if(button == 2){
					if(state == 0 && GS.gunOneLoaded >= 2){
						GS.charging = true;
					}
					if(state == 1 && GS.charging){
						GS.charging = false;
						fire(.1 + (GS.charge * .004));
						GS.gunOneLoaded -= 2;
						GS.delay = GS.gunOneDelay;
						GS.charge = 0;
					}
				}
				break;
			case 1:
				if(state == 0 && GS.gunTwoAmmo > 0){
					GS.charging = true;
				}
				if(state == 1){
					GS.charging = false;
				}
				break;
			case 2:
				if(GS.gunThreeAmmo > 0 && GS.delay == 0){
					fire(.1);
					GS.gunThreeAmmo--;
					GS.delay = GS.gunThreeDelay;
				}
				break;
			break;
		}
	}
}

// Stuff we do a lot that doesn't have to do with drawing
void movement(){
	size_t lasttime = glutGet(GLUT_ELAPSED_TIME);
	float last_ypos;
	for(;;){
		if(GS.delay != 0){
			GS.delay--;
		}
		if(GS.charging){
			if(GS.charge < 100){
				GS.charge += 1;
			}
		}
		if(GS.charging && GS.equiped == 1 && GS.gunTwoAmmo > 0){
			fire(.1);
			GS.gunTwoAmmo -= 1;
		}
		if(GS.jump_delay > 0){
			GS.jump_delay--;
		}
		size_t etime = glutGet(GLUT_ELAPSED_TIME) - lasttime;
		movement_loops++;
		last_ypos = ypos;
		ypos += yv * etime; // Because yv was in 1/60 second intervals
		yv -= 0.00008 * etime;
		float floor = -1;
		handle_keys_down(etime);
		
		for(auto w : objects)
			if(w->block) {
				BlockObject* bo = dynamic_cast<BlockObject*>(w);
				if(bo->over(xpos, zpos)) {
					register float floorcand = bo->size_y/2 + w->y;			
					if(floorcand < ypos+.25 && floorcand > last_ypos){
						ypos = last_ypos;
						yv = 0;
					} else if(floorcand > floor && floorcand < ypos+.25)
						floor = floorcand;
				}
				if(w->overlap(next_xpos, last_ypos, next_zpos, .2)){
					char blockdir = bo->blockDirection(xpos, last_ypos, zpos, next_xpos, ypos, next_zpos, .2);
					if(blockdir == 'x')
						next_xpos = xpos;
					if(blockdir == 'z')
						next_zpos = zpos;
				}
			}
		for(auto w : pickups)
			if(w->block) {
				BlockObject* bo = dynamic_cast<BlockObject*>(w);
				if(bo->over(xpos, zpos)) {
					register float floorcand = bo->size_y/2 + w->y;			
					if(floorcand < ypos+.25 && floorcand > last_ypos){
						ypos = last_ypos;
						yv = 0;
					} else if(floorcand > floor && floorcand < ypos+.25)
						floor = floorcand;
				}
				if(w->overlap(next_xpos, last_ypos, next_zpos, .2)){
					char blockdir = bo->blockDirection(xpos, last_ypos, zpos, next_xpos, ypos, next_zpos, .2);
					if(blockdir == 'x')
						next_xpos = xpos;
					if(blockdir == 'z')
						next_zpos = zpos;
				}
			}
		for(auto w : Door)
			if(w->block) {
				BlockObject* bo = dynamic_cast<BlockObject*>(w);
				if(bo->over(xpos, zpos)) {
					register float floorcand = bo->size_y/2 + w->y;			
					if(floorcand < ypos+.25 && floorcand > last_ypos){
						ypos = last_ypos;
						yv = 0;
					} else if(floorcand > floor && floorcand < ypos+.25)
						floor = floorcand;
				}
				if(w->overlap(next_xpos, last_ypos, next_zpos, .2)){
					char blockdir = bo->blockDirection(xpos, last_ypos, zpos, next_xpos, ypos, next_zpos, .2);
					if(blockdir == 'x')
						next_xpos = xpos;
					if(blockdir == 'z')
						next_zpos = zpos;
				}
			}

		xpos = next_xpos;
		zpos = next_zpos;
		if(ypos < floor+1){
			ypos = floor+1;
			yv = 0;
		}

		for(GameObject *o : objects) 
			if(!o->dead)
				o->cycle(etime);

		for(auto o : objects)
			if(!o->dead)
				o->collisions();

		if(!invincible) {
			GameObject* collider = radial_collision(xpos, ypos, zpos, 0.25);
			if(collider){
				if(GS.armor > 0){
					GS.armor -= 1;
				} else {
					GS.hp -= 2;
				}
				if(GS.hp <= 0){
					hit = 1;			
					float* lap = getHeadingCoordinates(PI);
					xpos += lap[0] * SPEED * etime;
					zpos += lap[2] * SPEED * etime;
					free(lap);
				}
			}
		}

		objects_lock.lock();
		for(GameObject *o : object_additions)
			objects.push_back(o);
		for(int i = 0; i < objects.size(); i++)
			if(objects[i]->dead){
				delete objects[i];
				objects[i] = objects[objects.size() - 1];
				objects.pop_back();
			}
		objects_lock.unlock();
		object_additions.clear();

		int loop_runtime = (glutGet(GLUT_ELAPSED_TIME) - lasttime - etime);
		lasttime += etime;
		if(loop_runtime < 9)
			usleep(10000 - loop_runtime * 1000);
		if(done)
			break;
	}
	printf("Movement loop is quitting\n");
}

/*  Create checkerboard texture  */
#define checkImageWidth 64
#define checkImageHeight 64
static GLubyte checkImage[checkImageHeight][checkImageWidth][4];


void makeCheckImage(void)
{
   int i, j, c;
    
   for (i = 0; i < checkImageHeight; i++) {
      for (j = 0; j < checkImageWidth; j++) {
         c = ((((i&0x8)==0)^((j&0x8))==0))*255;
         checkImage[i][j][0] = (GLubyte) c;
         checkImage[i][j][1] = (GLubyte) c;
         checkImage[i][j][2] = (GLubyte) c;
         checkImage[i][j][3] = (GLubyte) 200;
      }
   }
}

void init(void)
{    
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);
   glEnable(GL_DEPTH_TEST);

   makeCheckImage();
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glGenTextures(1, &texName);
   glBindTexture(GL_TEXTURE_2D, texName);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                   GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                   GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, checkImageWidth, 
                checkImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 
                checkImage);
}

void resize(int w, int h){
	width = w;
	height = h;
	glViewport(0, 0, width, height);
}

int main(int argc, char ** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(width, height);
	glutCreateWindow("Simple OpenGL Demo");
	glEnable(GL_DEPTH_TEST);
	glutIdleFunc(drawScene);
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(keypress);
	glutKeyboardUpFunc(keyup);
	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	glutPassiveMotionFunc(mousemove);
	glutMotionFunc(mousemove);
	glutMouseFunc(mousefire);
	glutSetCursor(GLUT_CURSOR_NONE);
   init();

	
	float material_color[] = {0.5, 1.0, .5, 1.0};
	float material_spec_color[] = {.8, .8, 1, 1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material_color);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_spec_color);
	
	float light0position[] = {5, 3, 0};
	glLightfv(GL_LIGHT0, GL_POSITION, light0position);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	float light1color[] = {0.5, 0.5, 0.5, 1.0};
	float light1position[] = {0, 500, 0};
	float down[] = {0, -1, 0};
	glLightfv(GL_LIGHT1, GL_AMBIENT_AND_DIFFUSE, light1color);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1color);
	glLightfv(GL_LIGHT1, GL_POSITION, light1position);
	glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, down);
	glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 5);
	glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, 1);
	glEnable(GL_LIGHT1);


	objects.reserve(500);
	objects.push_back(new Target(0, 0, 13, drawTarget, 0));
	objects.push_back(new Target(7, 12, 27, drawTarget, 11.375));
	objects.push_back(new Target(7, 12, 33, drawTarget, 11.375));
	objects.push_back(new Target(9, 13, 30, drawTarget, 11.375));
	objects.push_back(new Target(13, 12, 26, drawTarget, 11.375));
	objects.push_back(new Target(20, 12, 26, drawTarget, 11.375));
	objects.push_back(new Target(20, 13, 27, drawTarget, 11.375));
	objects.push_back(new Target(30, 12, 26, drawTarget, 11.375));
	objects.push_back(new Target(35, 12, 27, drawTarget, 11.375));
	objects.push_back(new Target(40, 13, 30, drawTarget, 11.375));
	objects.push_back(new Target(47, 13, 27, drawTarget, 11.375));
	objects.push_back(new Target(47, 12.5, 35, drawTarget, 11.375));
	objects.push_back(new ChaseTarget(23, 4.35, 9, drawAnotherTarget, 4.35, true, 20));
	objects.push_back(new ChaseTarget(47, 4.35, 9, drawAnotherTarget, 4.35, true, 20));
	objects.push_back(new ChaseTarget(30, 4.35, 9, drawAnotherTarget, 4.35, true, 20));
	objects.push_back(new ChaseTarget(36, 12, 30, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(30, 12, 25, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(30, 12, 30, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(36, 12, 20, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(1, 12, 7, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(1, 12, 10, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(1, 12, 20, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new ChaseTarget(1, 12, 30, drawAnotherTarget, 11.375, true, 20));
	objects.push_back(new SwingTarget(1, 21.5, 0, drawTarget, 15.65));
 

	
	WallType *brick_wall = new WallType("brick.jpg", material_color, material_spec_color);
	//initial wall
	objects.push_back(new WallTile(-1, 14, 22, .1, 35, 50, brick_wall));
	objects.push_back(new WallTile(24, 14, -3, 50, 35, .1, brick_wall));
	objects.push_back(new WallTile(49, 14, 22, .1, 35, 50, brick_wall));
	objects.push_back(new WallTile(24, 14, 47, 50, 35, .1, brick_wall));
	//bottom floor
	objects.push_back(new WallTile(1, 0, .5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(1, 0, 2.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(1, 0, 4.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(1, 0, 6.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(8, 0, .5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(8, 0, 2.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(8, 0, 4.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(8, 0, 6.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(10, 4, 17, .1, 10, 40, brick_wall));
	objects.push_back(new WallTile(1, 0, 15.5, 4, 4, .1, brick_wall));
	objects.push_back(new WallTile(8, 0, 15.5, 4, 4, .1, brick_wall));
	//first floor
	objects.push_back(new WallTile(4.5, 2, 6.2, 11, .1, 18.5, brick_wall));
	objects.push_back(new WallTile(14, 4, 37, 8, 11, .1, brick_wall));
	objects.push_back(new WallTile(33, 3.3, 42.5, 31.5, .1, 11, brick_wall));
	objects.push_back(new WallTile(29.5, 3.3, 17, 40, .1, 40, brick_wall));
	objects.push_back(new WallTile(18, 6, 31, .1, 6, 12, brick_wall));
	//second floor
	objects.push_back(new WallTile(24, 10.325, 26, 50, .1, 42, brick_wall));
	objects.push_back(new WallTile(12, 10.325, .75, 27, .1, 8.5, brick_wall));
	objects.push_back(new WallTile(47, 10.325, 1, 7, .1, 8, brick_wall));
	objects.push_back(new WallTile(28, 12.825, 5, 30, 5, .1, brick_wall));
	objects.push_back(new WallTile(38, 12.825, 10, 23, 5, .1, brick_wall));
	objects.push_back(new WallTile(25, 12.825, 1, .1, 5, 8, brick_wall));
	fake_objects.push_back(new WallTile(5.5, 12.875, 5, 15, 5, .1, brick_wall));
	objects.push_back(new WallTile(30, 12.825, 25, 38, 5, .1, brick_wall));
	objects.push_back(new WallTile(16, 12.825, 35, 38, 5, .1, brick_wall));
	//third floor
	objects.push_back(new WallTile(25, 15.65, 16, 55, .1 , 38 , brick_wall));
	objects.push_back(new WallTile(33.5, 15.65, 41, 31, .1 , 12 , brick_wall));
	objects.push_back(new WallTile(3, 15.65, 41, 9, .1 , 12 , brick_wall));
	objects.push_back(new WallTile(22.5, 29.65, 22, .1, 20 , 4 , brick_wall));
	objects.push_back(new WallTile(26.5, 29.65, 22, .1, 20 , 4 , brick_wall));
	objects.push_back(new WallTile(24.5, 29.65, 20, 4, 20 , .1 , brick_wall));
	objects.push_back(new WallTile(24.5, 28.65, 24, 4, 18 , .1 , brick_wall));
	objects.push_back(new WallTile(24.5, 39.65, 22, 4, .1 , 4 , brick_wall));
	objects.push_back(new WallTile(10, 21.5, 22, 25, .1 , 52 , brick_wall));
	objects.push_back(new WallTile(39, 21.5, 22, 25, .1 , 52 , brick_wall));
	objects.push_back(new WallTile(24.5, 21.5, 36, 4, .1 , 24 , brick_wall));
	objects.push_back(new WallTile(24.5, 21.5, 8, 4, .1 , 24 , brick_wall));
	for( int i = 3; i < 30; i += 5){
		objects.push_back(new WallTile(1, 17.75, i, 4, 4, .1, brick_wall));
		objects.push_back(new SwingTarget(1, 21.5, i-3, drawTarget, 15.65));
		objects.push_back(new WallTile(47, 17.75, i, 4, 4, .1, brick_wall));
		objects.push_back(new SwingTarget(47, 21.5, i-3, drawTarget, 15.65));
	}
		
//	for(float i = 0; i < 20; i++)
//		objects.push_back(new WallTile(-i - 6, (20 - i)/10 - 1, 0, 1, .5, 1, brick_wall));

	for(float i = 0; i < 15; i++){
		// Section at -3, 3, -5
		float next_y = ((-1 + i) *.2) -1;
		float next_z = 23 + i;
		objects.push_back(new WallTile(4.5, next_y, next_z, 11, .3, 1, brick_wall));
	}
	objects.push_back(new WallTile(4.5, 1.8, 43, 11, .2, 11, brick_wall));

	for(float i = 0; i < 8; i++){
		// Section at -3, 3, -5
		float next_y = ((-1 + i) *.2) + 2;
		float next_x = 10 + i;
		objects.push_back(new WallTile(next_x, next_y, 41.5, 1, .3, 11, brick_wall));
	}

	for(float i = 0; i < 14; i++){
		// Section at -3, 3, -5
		float next_y = ((4.35 + i) *.5) + 1.5;
		float next_x = 30 + i;
		objects.push_back(new WallTile(next_x, next_y, 0, 1, .3, 11, brick_wall));
	}
	for(float i = 0; i < 11; i++){
		// Section at -3, 3, -5
		float next_y = ((18 + i) *.5) + 1.5;
		float next_x = 18 - i;
		objects.push_back(new WallTile(next_x, next_y, 42, 1, .3, 14, brick_wall));
	}
//	objects.push_back(new WallTile(-2.5 + 20 + 3, 3 + 20 * .2, -5, 7, 1, 7, brick_wall));
	
//	objects.push_back(new SimpleGameObject(2, 0, -1, "ball.obj", "wood.jpg"));
	WallType *wood_wall = new WallType("wood.jpg", material_color, material_spec_color);
	pickups.push_back(new ItemBox(9, -.75, 3.5, .5, .5, .5, wood_wall, 0));
	pickups.push_back(new ItemBox(8, -.75, 11, .5, .5, .5, wood_wall, 1));
	pickups.push_back(new ItemBox(45, 11, .5, .5, .5, .5, wood_wall, 4));
	pickups.push_back(new ItemBox(1, 16, 0, .5, .5 , .5 , wood_wall, 6));
	pickups.push_back(new ItemBox(0, 16, 16, .5, .5 , .5 , wood_wall, 7));
	pickups.push_back(new ItemBox(46, 16, .5, .5, .5 , .5 , wood_wall, 7));
	pickups.push_back(new ItemBox(45, 16, 44, .5, .5 , .5 , wood_wall, 7));
	WallType *metal_wall = new WallType("metal.jpg", material_color, material_spec_color);
	Door.push_back(new Gate(4.5 , 0, 15.5, 4, 4, .1, metal_wall, 1));
	Door.push_back(new Gate(22.5, 17.65, 22, .1, 4 , 4 , metal_wall, 2));
	Door.push_back(new Gate(26.5, 17.65, 22, .1, 4 , 4 , metal_wall, 2));
	Door.push_back(new Gate(24.5, 17.65, 20, 4, 4 , .1 , metal_wall, 2));
	Door.push_back(new Gate(24.5, 17.65, 24, 4, 4 , .1 , metal_wall, 2));
	objects.push_back(new Lift(24.5, 16, 22, 4, .1, 4, metal_wall, 37.65));
	WallType *arm_wall = new WallType("armor.png", material_color, material_spec_color);
	pickups.push_back(new ItemBox(8, 3-.75, -.5, .5, .5, .5, arm_wall, 3));
	pickups.push_back(new ItemBox(22, 11.375-.75, -1, .5, .5, .5, arm_wall, 3));
	WallType *cross_wall = new WallType("red-cross.jpg", material_color, material_spec_color);
	pickups.push_back(new ItemBox(15.3, 4.35-.75, 34, .5, .5, .5, cross_wall, 2));
	pickups.push_back(new ItemBox(22, 11.375-.75, 4, .5, .5, .5, cross_wall, 2));
	pickups.push_back(new ItemBox(33, 11.375-.75, 47, .5, .5, .5, cross_wall, 2));
	pickups.push_back(new ItemBox(30, 11.375-.75, 47, .5, .5, .5, cross_wall, 2));
	WallType *bullet_wall = new WallType("bullet.jpg", material_color, material_spec_color);
	pickups.push_back(new ItemBox(22, 11.375-.75, 2, .5, .5, .5, bullet_wall, 5));
	WallType *special = new WallType("Filler2.png", material_color, material_spec_color);
	objects.push_back(new WallTile(24.9, 12.825, 1, .1, 2.5, 4, special));
	WallType *special2 = new WallType("Filler.png", material_color, material_spec_color);
	objects.push_back(new WallTile(24.5, 22.825, 46, 4, 2.5, .1, special2));
	objects.push_back(new SimpleTarget (26, 22, 40, "dalek.obj", "dalek.mtl", 37));   
	thread calc_fps([](){
		for(;;){
			sleep(1);
			fps = framecount;
			framecount = 0;
			etime_average = (1000.0 / movement_loops);
			movement_loops = 0;
		}
	});
	move_thread = new thread(movement);
	debug_output("Starting loop with %d objects\n", objects.size());
	
	glutMainLoop();
	return 0;
}



