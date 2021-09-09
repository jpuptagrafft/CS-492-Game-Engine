#include<GL/gl.h>
#include<GL/glu.h>
#include<GL/glut.h>
#include<SOIL/SOIL.h>
#include<math.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<thread>
#include<vector>
#include "game.h"




GameObject* radial_collision(float x, float y, float z, float radius, GameObject* o) {
	for(auto proj : objects) {
		if(proj == o || !proj->dangerous || proj->dead)
			continue;
		Projectile* project = dynamic_cast<Projectile*>(proj);
		if(proximity(x, proj->x, y, proj->y, z, proj->z) < project->size + radius)
			return proj;
	}
	return 0;
}

inline GameObject* radial_collision(GameObject* o, float radius){
	radial_collision(o->x, o->y, o->z, radius, o);
}

unsigned load_texture(const char* texfile){
	int width, height;
	unsigned texname;
	unsigned char* image = SOIL_load_image(texfile, &width, &height, 0, SOIL_LOAD_RGB);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glGenTextures(1, &texname);
   glBindTexture(GL_TEXTURE_2D, texname);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	return texname;
}

void Projectile::draw(){
	float material_color[] = {0.6, 1.0, 0.6, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material_color);
	float material_spec_color[] = {0.6, 1.0, 0.6, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_spec_color);
	glPushMatrix();
	glTranslatef(x, y, z);
	glutSolidSphere(size, 10, 10);
	glPopMatrix();
}

void Projectile::cycle(size_t etime){
	if(velocity > 0){
		x = move_x(heading, x, velocity * etime, elevation); 
		y = move_y(elevation, y, velocity * etime);
		if(y < -1)
			elevation = -elevation;
		z = move_z(heading, z, velocity * etime, elevation);
	}
	time_remaining -= etime;
	if(time_remaining <= 0) 
		dead = true;
}

void ExpandingProjectile::cycle(size_t etime){
		if(velocity > 0){
		x = move_x(heading, x, velocity * etime, elevation); 
		y = move_y(elevation, y, velocity * etime);
		if(y < -1)
			elevation = -elevation;
		z = move_z(heading, z, velocity * etime, elevation);
	}
	time_remaining -= etime;
	if(velocity == -1){
		if (size < maxExpand){
			size += .1;
		}
	}
	if(time_remaining <= 0) 
		dead = true;
}

void ExpandingProjectile::collisions(){
	float next_x = move_x(heading, x, velocity * 10, elevation);
	float next_y = move_y(elevation, y, velocity * 10);
	float next_z = move_z(heading, z, velocity * 10, elevation);
	for(auto w : objects)
		if(w->block)
			if(w->overlap(next_x, next_y, next_z, size)) {
				velocity = -1;
			}
}


void Projectile::collisions(){
	float next_x = move_x(heading, x, velocity * 10, elevation);
	float next_y = move_y(elevation, y, velocity * 10);
	float next_z = move_z(heading, z, velocity * 10, elevation);
	for(auto w : objects)
		if(w->block)
			if(w->overlap(next_x, next_y, next_z, size)) {
				velocity = -1;
				dangerous = false;
			}
}

void Frag::draw(){
	float material_color[] = {0.5, 0.5, 1.0, 1.0};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, material_color);
	float material_spec_color[] = {.8, .8, 1, 1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material_spec_color);

	glTranslatef(x, y, z);
	glutSolidSphere(0.1, 8, 8);
	glTranslatef(-z, -y, -z);
}

void Frag::move(){
	yv -= 0.005;
	x += xv;
	y += yv;
	z += zv;

	if(y < -1 - ty){
		yv = 0.8 * -yv;
		xv = 0.8 * xv;
		zv = 0.8 * zv;
	}	
}

void Target::draw(){
	glPushMatrix();
	glTranslatef(x, y, z);
	if(destroyed_count)
		for(auto & element : fragments)
			element.draw();
	else drawCallback();
	glPopMatrix();
}

void Target::cycle(size_t etime) {
	if (ypos >= trigger - .5 && ypos <= trigger + .5)
			shootback = true;
	if(shootback){
		if(destroyed_count){				
			if(destroyed_count == 1)
				for(int i = 0; i < nFragments; i++)
					fragments.push_back(Frag(y));
			for(auto & element : fragments)
				element.move();
			destroyed_count++;
		}
		if(!destroyed_count){
			if(random() % 50 != 42)
				goto no_fire;
			// Shoot from x, y, z to xpos, ypos, zpos
			if(fabs(x - xpos) <  0.01)
				goto no_fire;
			if(fabs(z - zpos) <  0.01)
				goto no_fire;
		
			float dx = xpos - x, dz = zpos - z;
			float p_heading = atanf(fabs(dz) / fabs(dx));
			if(dx > 0 && dz > 0) // Quadrant 1
				p_heading = PI/2 - p_heading + PI;
			else if(dx < 0 && dz > 0) // Quadrant 2
				p_heading = p_heading + PI/2;
			else if(dx < 0 && dz < 0) // Quadrant 3
				p_heading = 3*PI/2 - p_heading + PI;
			else if(dx > 0 && dz < 0) // Quadrant 4
				p_heading = p_heading + 3*PI/2; 
			else goto no_fire;
			float r = sqrt(pow(dz, 2) + pow(dx, 2));
			float dy = ypos - y;
			float p_elevation = 0;
			if(fabs(dy) > 0.01){
				p_elevation = atanf(fabs(dy) / r) * ((dy < 0)? -1:1);
			}
			Projectile* newp = new Projectile(x, y, z, p_heading, p_elevation, .1);
			newp->cycle(100);
			object_additions.push_back(newp);
		}
	}
no_fire:
	if(destroyed_count >= 420) 
		dead = true;
}

void SimpleTarget::draw(){
	glPushMatrix();
	glTranslatef(x, y, z);
	if(destroyed_count)
		for(auto & element : fragments)
			element.draw();
	else model.draw();
	glPopMatrix();
}

void SimpleTarget::cycle(size_t etime) {
	if (ypos >= trigger - .5 && ypos <= trigger + .5)
			shootback = true;
	if(shootback){
		if(destroyed_count){				
			if(destroyed_count == 1)
				for(int i = 0; i < nFragments; i++)
					fragments.push_back(Frag(y));
			for(auto & element : fragments)
				element.move();
			destroyed_count++;
		}
		if(!destroyed_count){
			if(random() % 50 != 42)
				goto no_fire;
			// Shoot from x, y, z to xpos, ypos, zpos
			if(fabs(x - xpos) <  0.01)
				goto no_fire;
			if(fabs(z - zpos) <  0.01)
				goto no_fire;
		
			float dx = xpos - x, dz = zpos - z;
			float p_heading = atanf(fabs(dz) / fabs(dx));
			if(dx > 0 && dz > 0) // Quadrant 1
				p_heading = PI/2 - p_heading + PI;
			else if(dx < 0 && dz > 0) // Quadrant 2
				p_heading = p_heading + PI/2;
			else if(dx < 0 && dz < 0) // Quadrant 3
				p_heading = 3*PI/2 - p_heading + PI;
			else if(dx > 0 && dz < 0) // Quadrant 4
				p_heading = p_heading + 3*PI/2; 
			else goto no_fire;
			float r = sqrt(pow(dz, 2) + pow(dx, 2));
			float dy = ypos - y;
			float p_elevation = 0;
			if(fabs(dy) > 0.01){
				p_elevation = atanf(fabs(dy) / r) * ((dy < 0)? -1:1);
			}
			Projectile* newp = new Projectile(x, y, z, p_heading, p_elevation, .1);
			newp->cycle(100);
			object_additions.push_back(newp);
		}
	}
no_fire:
	if(destroyed_count >= 420) 
		dead = true;
}

void ChaseTarget::cycle(size_t etime) {
	if (ypos >= trigger - .5 && ypos <= trigger + .5){
		if(triggerx && xpos > Space)
			shootback = true;
		if(triggerz && zpos > Space)
			shootback = true;
	}
	if(shootback){
		if(destroyed_count){				
		if(destroyed_count == 1)
			for(int i = 0; i < nFragments; i++)
				fragments.push_back(Frag(y));
		for(auto & element : fragments)
			element.move();
		destroyed_count++;
		}
		if(!destroyed_count){
			if (x > xpos){
				x-= .05;
			} 
			if (x < xpos) {
				x += .05;
			}
			if (z > zpos){
				z -= .05;
			} 
			if (z < zpos) {
				z += .05;
			}
			if(random() % 50 != 42)
				goto no_fire;
			// Shoot from x, y, z to xpos, ypos, zpos
			if(fabs(x - xpos) <  0.01)
				goto no_fire;
			if(fabs(z - zpos) <  0.01)
				goto no_fire;
		
			float dx = xpos - x, dz = zpos - z;
			float p_heading = atanf(fabs(dz) / fabs(dx));
			if(dx > 0 && dz > 0) // Quadrant 1
				p_heading = PI/2 - p_heading + PI;
			else if(dx < 0 && dz > 0) // Quadrant 2
				p_heading = p_heading + PI/2;
			else if(dx < 0 && dz < 0) // Quadrant 3
				p_heading = 3*PI/2 - p_heading + PI;
			else if(dx > 0 && dz < 0) // Quadrant 4
				p_heading = p_heading + 3*PI/2; 
			else goto no_fire;
			float r = sqrt(pow(dz, 2) + pow(dx, 2));
			float dy = ypos - y;
			float p_elevation = 0;
			if(fabs(dy) > 0.01){
				p_elevation = atanf(fabs(dy) / r) * ((dy < 0)? -1:1);
			}
			Projectile* newp = new Projectile(x, y, z, p_heading, p_elevation, .1);
			newp->cycle(100);
			object_additions.push_back(newp);
		}
	no_fire:
	if(destroyed_count >= 420) 
		dead = true;		
	}
}

void Target::collisions(){
	GameObject* other = radial_collision(this, 0.25);
	if(other && destroyed_count == 0)
		destroyed_count = 1;
}

void SimpleTarget::collisions(){
	GameObject* other = radial_collision(this, .5);
	if(other)
		HP -= 2;
		if(HP <=0 && destroyed_count == 0)
			destroyed_count = 1;
}

void SwingTarget::cycle(size_t etime){
	Target::cycle(etime);
	if(destroyed_count) return;
	angle += shift;
	if(angle < PI/4)
		shift += 0.001;
	if(angle > PI/4)
		shift -= 0.001;
	float effective_angle = angle - PI/4 - PI/2;
	float dx = 2 * cosf(effective_angle);
	float dy = 2 * sinf(effective_angle);
	x = base_x + dx;
	y = base_y + dy;
}

void SwingTarget::draw(){
	Target::draw();
	if(!destroyed_count){
		glBegin(GL_LINES);
		glVertex3f(x, y, z);
		glVertex3f(base_x, base_y, z);
		glEnd();
	}
}

void SquishyBall::draw(){
	glPushMatrix();
	if(animation_time >= 0){
		// 0:  Looks normal
		// 1:  Looks normal
		// 0.5:  Maximum flat
		register float a = fabs(0.5 - animation_time);
		register float d = cos(a * 3);
		register float w = 1.0 + d;
		register float h = 1.0 - 0.5 * d;
		glScalef(w, h, w);
		glTranslatef(x, y - d * 1.3, z);
	} else {
		glTranslatef(x, y, z);

	}
	glutSolidSphere(.5, 10, 10);
	glPopMatrix();

}

void SquishyBall::cycle(size_t etime){
	if(animation_time >= 0){
		animation_time += 0.01;
		if(animation_time > 1.0)
			animation_time = -1;
		return;
	}
	y += yvel;
	yvel -= 0.001;
	if(y <= -0.5){
		yvel = -yvel;
		y = -0.5;
		animation_time = 0;
	}
}

void ItemBox::interact(){
	switch(trigger){
		case 0:
			if (!GS.hasGun){
				GS.hasGun = true;
			}
			GS.gunOne = true;
			GS.equiped = 0;
			break;
		case 1:
			GS.keyOne = true;
			for(int t = 0; t < Door.size(); t++){
				if(Door[t]->trigger == 1){
					delete Door[t];
					Door[t] = Door[Door.size() - 1];
					Door.pop_back();
				}
			}
			break;

		case 2:
			if(GS.hp + 20 > 100){
				GS.hp = 100;
			} else {
				GS.hp += 20;
			}
			break;
		case 3:
			if(GS.armor + 10 > 50){
				GS.armor = 50;
			} else {
				GS.armor += 10;
			}
			break;
		case 4:
			if (!GS.hasGun){
				GS.hasGun = true;
			}
			GS.gunTwo = true;
			GS.equiped = 1;
			break;
		case 5:
			switch(GS.equiped){
				case 0:
					GS.gunOneAmmo += 8;
					break;
				case 1:
					GS.gunTwoAmmo += 100;
					break;
				case 2:
					GS.gunThreeAmmo += 2;
				break;
			}
			break;
		case 6:
			if (!GS.hasGun){
				GS.hasGun = true;
			}
			GS.gunThree = true;
			GS.equiped = 2;
			break;
		case 7:
			GS.keyPTwo++;
			if(GS.keyPTwo == 3){
				for(int t = 0; t < Door.size(); t++){
					if(Door[t]->trigger == 2){
						delete Door[t];
						Door[t] = Door[Door.size() - 1];
						Door.pop_back();
					}
				}
			}
			break;
	}
}			
bool ItemBox::overlap(float ox, float oy, float oz, float margin){
	float dx = fabs(ox - x);
	float dy = fabs(oy - y);
	float dz = fabs(oz - z);
	if(dx < size_x/2 + margin)
		if(dy < size_y/2 + margin)
			if(dz < size_z/2 + margin)
				return true;
	return false;
}

bool ItemBox::over(float ox, float oz) {
	float dx = fabs(ox - x);
	float dz = fabs(oz - z);
	if(dx < size_x/2)
		if(dz < size_z/2)
				return true;
	return false;
}

char ItemBox::blockDirection(float ox, float oy, float oz, float nx, float ny, float nz, float margin){
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

void Lift::cycle(size_t etime){
	if (xpos > x - 2 && xpos < x + 2 && zpos > z - 2 && zpos < z + 2 && ypos > y - 2 && ypos < y + 2){
		if(y < stop)
			y += .1;
	} else {
		if(y > inY)
			y -= .1;
	}
}



