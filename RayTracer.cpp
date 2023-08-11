/*==================================================================================
* COSC 363  Computer Graphics (2023)
* Department of Computer Science and Software Engineering, University of Canterbury.
*
* A basic ray tracer
* See Lab06.pdf   for details.
*===================================================================================
*/
#include <iostream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include "Sphere.h"
#include "Cylinder.h"
#include "Plane.h"
#include "SceneObject.h"
#include "Ray.h"
#include <GL/freeglut.h>
#include "TextureBMP.h"
using namespace std;

const float EDIST = 40.0;
const int NUMDIV = 500;
const int MAX_STEPS = 5;
const float XMIN = -10.0;
const float XMAX = 10.0;
const float YMIN = -10.0;
const float YMAX = 10.0;
const float PI = 3.14159265359;
const bool ANTI_ALIAS = true;

TextureBMP texture;

vector<SceneObject*> sceneObjects;


//---The most important function in a ray tracer! ---------------------------------- 
//   Computes the colour value obtained by tracing a ray and finding its 
//     closest point of intersection with objects in the scene.
//----------------------------------------------------------------------------------
glm::vec3 trace(Ray ray, int step)
{
	glm::vec3 backgroundCol(0);						//Background colour = (0,0,0)
	glm::vec3 lightPos(29, 29, -3);					//Light's position
	glm::vec3 lightPos2(-29, 29, -3);				//Light2's position
	glm::vec3 color(0);
	SceneObject* obj;


    ray.closestPt(sceneObjects);					//Compare the ray with all objects in the scene
    if(ray.index == -1) return backgroundCol;		//no intersection
	obj = sceneObjects[ray.index];					//object on which the closest point of intersection is found

	if (ray.index == 0) // index 0 is floor plane
	{
		//Stripe pattern
		int stripeWidth = 5;
		int iz = (ray.hit.z) / stripeWidth;
		int ix = (ray.hit.x+100) / stripeWidth;
		int k = (iz+ix) % 2; //2 colors
		if (k == 0) color = glm::vec3(0.9, 0.9, 0.9);
		else color = glm::vec3(0.1, 0.1, 0.1);
		obj->setColor(color);


	}

	if (ray.index == 1) // back wall plane
	{
		//Stripe pattern
		int iz = (ray.hit.y)*2*sinf(ray.hit.x) + cosf(ray.hit.x);
		int k = iz % 2; //2 colors
		if (k == 0) color = glm::vec3(0.8, 0.8, 0.8);
		else color = glm::vec3(0.8, 0., 0.8);
		obj->setColor(color);

	}


	if (ray.index == 7) // textured sphere
	{
		glm::vec3 direction = sceneObjects[ray.index]->normal(ray.hit);

		//Mapping Texture to Sphere
		float texcoordu = 0.5 - atan2(direction.z, direction.x) / (2.0f * PI);
		float texcoordv = 0.5 + asin(direction.y) / PI;
		if (texcoordu > 0 && texcoordu < 1 &&
			texcoordv > 0 && texcoordv < 1)
		{
			color = texture.getColorAt(texcoordu, texcoordv);
			obj->setColor(color);
		}

	}

	color = 0.5f*obj->lighting(lightPos,-ray.dir,ray.hit) + 0.5f*obj->lighting(lightPos2, -ray.dir, ray.hit);						//Object's colour

	// SHADOWS

	glm::vec3 lightVec = lightPos - ray.hit;
	Ray shadowRay(ray.hit, lightVec);
	glm::vec3 lightVec2 = lightPos2 - ray.hit;
	Ray shadowRay2(ray.hit, lightVec2);

	glm::vec3 shadowcol(1);
	glm::vec3 shadowcol2(1);


	shadowRay.closestPt(sceneObjects);
	if (shadowRay.index > -1 && shadowRay.dist < ray.dist) {
		float ambient_scale_factor = 0.2;  //0.2 = ambient scale factor
		if (sceneObjects[shadowRay.index]->isTransparent()) ambient_scale_factor = 0.3;
		if (sceneObjects[shadowRay.index]->isRefractive()) ambient_scale_factor = 0.3;
		shadowcol = ambient_scale_factor * obj->getColor();
		color = shadowcol;
	}

	shadowRay2.closestPt(sceneObjects);
	if (shadowRay2.index > -1 && shadowRay2.dist < ray.dist) {
		float ambient_scale_factor = 0.2;  //0.2 = ambient scale factor
		if (sceneObjects[shadowRay2.index]->isTransparent()) ambient_scale_factor = 0.3;
		if (sceneObjects[shadowRay2.index]->isRefractive()) ambient_scale_factor = 0.3;
		shadowcol2 = ambient_scale_factor * obj->getColor();
		color = shadowcol2;

		color = min(shadowcol, shadowcol2);
	}

	// Special Conditions

	if (obj->isReflective() && step < MAX_STEPS)
	{
		float rho = obj->getReflectionCoeff();
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 reflectedDir = glm::reflect(ray.dir, normalVec);
		Ray reflectedRay(ray.hit, reflectedDir);
		glm::vec3 reflectedColor = trace(reflectedRay, step + 1);
		color = color + (rho * reflectedColor);
	}

	if (obj->isTransparent() && step < MAX_STEPS)
	{
		float tran_coeff = obj->getTransparencyCoeff();
		Ray transparentRay(ray.hit, ray.dir);
		glm::vec3 transparentColor = trace(transparentRay, step + 1);
		color = ((1- tran_coeff) * color) + ( tran_coeff * transparentColor);
	}

	if (obj->isRefractive() && step < MAX_STEPS) 
	{
		float refrac_coeff = obj->getRefractionCoeff();
		float refrac_index = obj->getRefractiveIndex();
		// Internal Ray
		glm::vec3 normalVec = obj->normal(ray.hit);
		glm::vec3 refractedDir = glm::refract(ray.dir, normalVec, refrac_index);
		Ray refractedRay(ray.hit, refractedDir);

		// Exiting Object
		normalVec = obj->normal(refractedRay.hit);
		refractedDir = glm::refract(refractedRay.dir, normalVec, refrac_index);
		Ray refractedRay2(refractedRay.hit, refractedDir);

		glm::vec3 refractedColor = trace(refractedRay2, step + 1);
		color = color + (refrac_coeff * refractedColor);
	}


    return color;
}

//---The main display module -----------------------------------------------------------
// In a ray tracing application, it just displays the ray traced image by drawing
// each cell as a quad.
//---------------------------------------------------------------------------------------
void display()
{
	float xp, yp;  //grid point
	float cellX = (XMAX - XMIN) / NUMDIV;  //cell width
	float cellY = (YMAX - YMIN) / NUMDIV;  //cell height
	glm::vec3 eye(0., 0., 0.);

	glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

	glBegin(GL_QUADS);  //Each cell is a tiny quad.

	for (int i = 0; i < NUMDIV; i++)	//Scan every cell of the image plane
	{
		xp = XMIN + i * cellX;
		for (int j = 0; j < NUMDIV; j++)
		{
			yp = YMIN + j * cellY;
			glm::vec3 col = glm::vec3(0);

			if (ANTI_ALIAS) {
				// Anti Aliasing
				glm::vec3 dir0(xp + 0.25 * cellX, yp + 0.25 * cellY, -EDIST);	//direction of the primary ray
				glm::vec3 dir1(xp + 0.25 * cellX, yp + 0.75 * cellY, -EDIST);	//direction of the primary ray
				glm::vec3 dir2(xp + 0.75 * cellX, yp + 0.25 * cellY, -EDIST);	//direction of the primary ray
				glm::vec3 dir3(xp + 0.75 * cellX, yp + 0.75 * cellY, -EDIST);	//direction of the primary ray

				Ray ray0 = Ray(eye, dir0);
				Ray ray1 = Ray(eye, dir1);
				Ray ray2 = Ray(eye, dir2);
				Ray ray3 = Ray(eye, dir3);

				glm::vec3 col0 = trace(ray0, 1); //Trace the primary ray and get the colour value
				glm::vec3 col1 = trace(ray1, 1); //Trace the primary ray and get the colour value
				glm::vec3 col2 = trace(ray2, 1); //Trace the primary ray and get the colour value
				glm::vec3 col3 = trace(ray3, 1); //Trace the primary ray and get the colour value

				col = 0.25f * col0 + 0.25f * col1 + 0.25f * col2 + 0.25f * col3;
			
			}
			else {
				// No Anti Aliasing
				glm::vec3 dir(xp + 0.5 * cellX, yp + 0.5 * cellY, -EDIST);	//direction of the primary ray
				Ray ray = Ray(eye, dir);
				col = trace(ray, 1); //Trace the primary ray and get the colour value */
			}

			// Standard
			glColor3f(col.r, col.g, col.b);
			glVertex2f(xp, yp);				//Draw each cell with its color value
			glVertex2f(xp + cellX, yp);
			glVertex2f(xp + cellX, yp + cellY);
			glVertex2f(xp, yp + cellY);
		}
	}

    glEnd();
    glFlush();
}



//---This function initializes the scene ------------------------------------------- 
//   Specifically, it creates scene objects (spheres, planes, cones, cylinders etc)
//     and add them to the list of scene objects.
//   It also initializes the OpenGL 2D orthographc projection matrix for drawing the
//     the ray traced image.
//----------------------------------------------------------------------------------
void initialize()
{
    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(XMIN, XMAX, YMIN, YMAX);

    glClearColor(0, 0, 0, 1);

	texture = TextureBMP("Earth.bmp");

	// BOUNDING CUBE

	Plane* floor = new Plane(glm::vec3(-30., -15, -40), //Point A
		glm::vec3(30., -15, -40), //Point B
		glm::vec3(30., -15, -200), //Point C
		glm::vec3(-30., -15, -200)); //Point D
	floor->setColor(glm::vec3(0.8, 0.8, 0));
	floor->setSpecularity(false);
	sceneObjects.push_back(floor);

	Plane* back_wall = new Plane(glm::vec3(-30., -15, -200), //Point A
		glm::vec3(30., -15, -200), //Point B
		glm::vec3(30., 30, -200), //Point C
		glm::vec3(-30., 30, -200)); //Point D
	back_wall->setColor(glm::vec3(0.8, 0.8, 0));
	back_wall->setSpecularity(false);
	sceneObjects.push_back(back_wall);

	Plane* left_wall = new Plane(glm::vec3(-30., -15, -40), //Point A
		glm::vec3(-30., -15, -200), //Point B
		glm::vec3(-30., 30, -200), //Point C
		glm::vec3(-30., 30, -40)); //Point D
	left_wall->setColor(glm::vec3(0, 0.8, 0.8));
	left_wall->setSpecularity(false);
	sceneObjects.push_back(left_wall);

	Plane* right_wall = new Plane(glm::vec3(30., -15, -200), //Point A
		glm::vec3(30., -15, -40), //Point B
		glm::vec3(30., 30, -40), //Point C
		glm::vec3(30., 30, -200)); //Point D
	right_wall->setColor(glm::vec3(0.8, 0., 0.));
	right_wall->setSpecularity(false);
	sceneObjects.push_back(right_wall);

	Plane* ceiling = new Plane(glm::vec3(-30., 30, -200), //Point A
		glm::vec3(30., 30, -200), //Point B
		glm::vec3(30., 30, -40), //Point C
		glm::vec3(-30., 30, -40)); //Point D
	ceiling->setColor(glm::vec3(0., 0.8, 0.));
	ceiling->setSpecularity(false);
	// ceiling->setReflectivity(true, 0.9);
	sceneObjects.push_back(ceiling);

	// MIRROR

	Plane* mirror = new Plane(glm::vec3(-30., -5, -180), //Point A
		glm::vec3(30., -5, -180), //Point B
		glm::vec3(30., 20, -180), //Point C
		glm::vec3(-30., 20, -180)); //Point D
	mirror->setColor(glm::vec3(0.5, 0.5, 0.5));
	mirror->setSpecularity(false);
	mirror->setReflectivity(true, 0.9);
	sceneObjects.push_back(mirror);

	// SPHERES

	Sphere* sphere_transp = new Sphere(glm::vec3(-13.0, 0.0, -90.0), 5.0);
	sphere_transp->setColor(glm::vec3(0, 0, 0.8));   //Set colour to blue
	sphere_transp->setTransparency(true, 0.8);
	sphere_transp->setShininess(50);
	sceneObjects.push_back(sphere_transp);		 //Add sphere to scene objects

	Sphere* sphere_tex = new Sphere(glm::vec3(0.0, 0.0, -60.0), 4.0); // index 7 
	sphere_tex->setColor(glm::vec3(0, 0, 1));   //Set colour to blue
	sceneObjects.push_back(sphere_tex);		 //Add sphere to scene objects

	Sphere* sphere_refrac = new Sphere(glm::vec3(13.0, 0.0, -90.0), 5.0);
	sphere_refrac->setColor(glm::vec3(0.8, 0.8, 0.8));   //Set colour to red
	sphere_refrac->setRefractivity(true, 1, 1.1);
	sphere_refrac->setTransparency(true, 0.8);
	sceneObjects.push_back(sphere_refrac);		 //Add sphere to scene objects

	// CYLINDERS

	Cylinder* cylinder1 = new Cylinder(glm::vec3(-13.0, -15.0, -90.0), 4, 10);
	cylinder1->setColor(glm::vec3(0., 0.8, 0.8));   //Set colour to blue
	sceneObjects.push_back(cylinder1);		 //Add sphere to scene objects

	Cylinder* cylinder2 = new Cylinder(glm::vec3(0.0, -15.0, -60.0), 3, 11);
	cylinder2->setColor(glm::vec3(0., 0.3, 0.8));   //Set colour to blue
	sceneObjects.push_back(cylinder2);		 //Add sphere to scene objects

	Cylinder* cylinder3 = new Cylinder(glm::vec3(13.0, -15.0, -90.0), 4, 10);
	cylinder3->setColor(glm::vec3(0., 0.8, 0.8));   //Set colour to blue
	sceneObjects.push_back(cylinder3);		 //Add sphere to scene objects

}


int main(int argc, char *argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(20, 20);
    glutCreateWindow("Raytracing");

    glutDisplayFunc(display);
    initialize();

    glutMainLoop();
    return 0;
}
