//#define DEBUG

#ifndef GAME_H
#define GAME_H

#include<vector>
#include<mutex>
#include<stdarg.h>
#include "tiny_obj_loader.h"
using namespace std;

#define CYCLEFREQ 10 // Milliseconds
#ifdef MAIN
#define EXTERN
#define INIT = 0
#else
#define EXTERN extern
#define INIT
#endif

#define PI 3.141592653



struct Keys {
	char forward;
	char backward;
	char stepright;
	char turnright;
	char stepleft;
	char turnleft;
	char fire;
};

class GameState {
	public:
		int hp = 100;
		int armor = 0;
		bool hasGun = false;
		bool gunOne = false;
		bool gunTwo = false;
		bool gunThree = false;
		bool keyOne = false;
		int keyPTwo = 0;
		int delay = 0;
		bool charging = false;
		int charge = 0;
		int equiped = 0;
		int gunOneClip = 8;
		int gunOneLoaded = 8;
		int gunOneAmmo = 64;
		int gunOneDelay = 30;
		int gunTwoAmmo = 500;
		int gunThreeDelay = 150;
		int gunThreeAmmo = 20;
		int jump_delay = 0;
};


class GameObject {
	public:
		float x, y, z;
		bool dangerous = false;
		bool dead = false;
		bool block = false;

		GameObject(float ix, float iy, float iz) : x(ix), y(iy), z(iz) {}
		virtual void draw() = 0;
		virtual void cycle(size_t etime) = 0;
		virtual void collisions() = 0;
		virtual bool overlap(float, float, float, float) = 0;
};

class LoadedObject {
	public:
		unsigned dl;
		tinyobj::attrib_t attributes;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		unsigned texname = 0;
		

		/*
		 * Loads object in the file "filename" into video memory
		 * Allocates a display list ID for the object
		 */
		LoadedObject(const char* filename);
		LoadedObject(const char* filename, const char* texturefile);

		~LoadedObject(){
			glDeleteLists(dl, 1);
		}

		void draw();
};

class BlockObject {
	public:
		float size_x, size_y, size_z;
		BlockObject(float isx, float isy, float isz) 
			: size_x(isx), size_y(isy), size_z(isz) {}

		virtual bool over(float ox, float oz) = 0;
		virtual char blockDirection(float ox, float oy, float oz, float nx, float ny, float nz, float margin) = 0;
};

class WallType {
	public:
		unsigned texname = 0;
		float material[4];
		float spec[4];
		unsigned dl;
		
		WallType(const char* texfile, float* im, float* is);
		void draw(float, float, float, float, float, float);	
};

class WallTile : public GameObject, public BlockObject {
	public:
		WallType *wt;
		WallTile(float ix, float iy, float iz, float isx, float isy, float isz, WallType* iwt) :
			GameObject(ix, iy, iz), BlockObject(isx, isy, isz) {wt = iwt; block = true;}

		void draw();
		void cycle(size_t etime) {};
		void collisions() {};
		bool overlap(float, float, float, float);
		bool over(float, float) override;
		char blockDirection(float, float, float, float, float, float, float);
};
class Lift : public WallTile{
	public:
		float stop;
		float inY;
		Lift(float ix, float iy, float iz, float isx, float isy, float isz, WallType* iwt, float s) :
			stop(s), inY(iy), WallTile(ix, iy, iz, isx, isy, isz, iwt) {};
		void cycle(size_t etime);
};

class ItemBox : public WallTile {
	public:
		int trigger;
		ItemBox(float ix, float iy, float iz, float isx, float isy, float isz, WallType* iwt, int t) : 
			trigger(t),  WallTile(ix, iy, iz, isx, isy, isz, iwt){}
		void interact();
		bool overlap(float, float, float, float);
		bool over(float, float) override;
		char blockDirection(float, float, float, float, float, float, float);
};
class Gate : public WallTile {
	public: 
		int trigger;
		Gate(float ix, float iy, float iz, float isx, float isy, float isz, WallType* iwt, int t) : 
			trigger(t), WallTile(ix, iy, iz, isx, isy, isz, iwt){}
};
EXTERN struct Keys kstatus;
EXTERN vector<GameObject*> objects;
EXTERN vector<LoadedObject*> models;
EXTERN vector<GameObject*> object_additions;
EXTERN vector<ItemBox*> pickups;
EXTERN vector<Gate*> Door;
EXTERN vector<GameObject*> fake_objects;
EXTERN mutex objects_lock;
EXTERN float xpos INIT, ypos INIT, zpos INIT, heading INIT, elevation INIT, yv INIT;
EXTERN int count INIT, framecount INIT, hit INIT;
EXTERN GameState GS;

/* We're not going to do this right now
EXTERN projectiles[1000];
EXTERN proj_count = 0;

void fire_projectile(.....){
	if(proj_count == 1000)
		either don't fire or reallocate more space
	projectiles[proj_count].Projectile(.....);
	object_additions.push_back(&projectiles[proj_count]);
	proj_count++;
}

void delete_projectile(i){
	 projectiles[i] = projectiles[proj_count] 
	 proj_count--;
}
*/

#ifdef DEBUG
inline void debug_output(char* format, ... ) {
	va_list arguments;                     
   va_start ( arguments, format );
	vprintf(format, arguments);
}
#else
inline void debug_output(char* format, ... ){
}
#endif

inline float move_x(float heading, float start, float distance, float elevation){
	return start + sin(PI+heading) * distance * cos(elevation);
}
inline float move_y(float elevation, float start, float distance){
	return start + sin(elevation) * distance;
}
inline float move_z(float heading, float start, float distance, float elevation){
	return start + sin((3*PI/2) + heading) * distance * cos(elevation);
}
inline float proximity(float x1, float x2, float y1, float y2, float z1, float z2){
	return sqrt(pow(x1-x2, 2) + pow(y1-y2, 2) + pow(z1-z2, 2));
}
GameObject* radial_collision(GameObject* o, float radius);
GameObject* radial_collision(float x, float y, float z, float radius, GameObject* o = (GameObject*)0);
unsigned load_texture(const char* texfile);

class Projectile : public GameObject {
	public:
		float heading, elevation;
		int time_remaining = 3000;
		float velocity = 0.01;
		float size;
		Projectile(float ix, float iy, float iz, float ih, float ie, float sz) 
			: GameObject(ix, iy, iz), heading(ih), elevation(ie), size(sz) {
			dangerous = true;
		}
		void draw() override;
		void cycle(size_t etime);
		void collisions() ;
		inline bool overlap(float ox, float oy, float oz, float margin) {
			return proximity(x, ox, y, oy, z, oz) < size + margin;
		}
};

class ExpandingProjectile : public Projectile {
	public:
		double maxExpand = 2.5;
		ExpandingProjectile(float ix, float iy, float iz, float ih, float ie, float sz) :
			Projectile(ix, iy, iz, ih, ie, sz) {}
		void cycle(size_t etime);
		void collisions();
};

class Frag {
	public:
		float x = 0, y = 0, z = 0;
		float ty;
		float xv, yv, zv;

		Frag(float ity) : ty(ity) {
			xv = random() % 100 / 2000.0 - 0.025;
			yv = random() % 100 / 400.0 - 0.01;
			zv = random() % 100 / 2000.0 - 0.025;
		}
		void draw();
		void move();
};

class Target : public GameObject {
	public:
		float size;
		float trigger;
		int nFragments = 20;
		float radius = 0.25;
		int destroyed_count = 0;
		vector<Frag> fragments;
		void (*drawCallback)();
		bool shootback = false;

		Target(float ix, float iy, float iz, void (*id)(), float t) 
			: GameObject(ix, iy, iz), trigger(t), drawCallback(id) {}

		void draw();
		void cycle(size_t etime);
		void collisions();
		inline bool overlap(float ox, float oy, float oz, float margin) {
			return proximity(x, ox, y, oy, z, oz) < 0.25 + margin;
		}
};

class ChaseTarget : public Target {
	public:
		bool triggerx, triggerz;
		float Space;
		ChaseTarget(float ix, float iy, float iz, void(*id)(), float e, bool isX, float space) :
			Target(ix, iy, iz, id, e) {
				if (isX){
					triggerx = true;
					triggerz = false;
					Space = space;
				} else {
					triggerz = true;
					triggerx = false;
					Space = space;
				}	
	};
		void cycle(size_t etime);
};

class SwingTarget : public Target {
	public:
		float base_x, base_y;
		float angle = 0.1, shift = 0.01;
		SwingTarget(float ix, float iy, float iz, void (*id)(), float t)
				: Target(ix, iy, iz, id, t), base_x(ix), base_y(iy) {}
		void cycle(size_t etime) override;
		void draw() override;
};

class SimpleGameObject : public GameObject {
	public:
		LoadedObject model;

		SimpleGameObject(float ix, float iy, float iz, const char* objfile, const char* texfile = 0);
		void draw() override;
		void cycle(size_t etime) override {};
		bool overlap(float ox, float oy, float oz, float margin) {return false;};
};

class SimpleTarget : public SimpleGameObject {
	public:
		int HP = 1000;
		float trigger;
		int nFragments = 20;
		float radius = 0.25;
		int destroyed_count = 0;
		bool shootback = false;
		vector<Frag> fragments;
		SimpleTarget(float ix, float iy, float iz, const char* objfile, const char* texfile = 0, float t = 0) : SimpleGameObject(ix, iy, iz, objfile, texfile), trigger(t) {}
		void draw();
		void cycle(size_t etime);
		void collisions();
		inline bool overlap(float ox, float oy, float oz, float margin) {
			return proximity(x, ox, y, oy, z, oz) < 0.25 + margin;
		}
	
};

class SquishyBall : public GameObject {
	public:
		float animation_time = -1;
		float yvel = 0;
		SquishyBall(float ix, float iy, float iz)
			: GameObject(ix, iy, iz) {};
		void draw();
		void cycle(size_t etime);
		void collisions() {};
		bool overlap(float, float, float, float) {};
};


#endif
