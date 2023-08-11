/*----------------------------------------------------------
* COSC363  Ray Tracer
*
*  The cylinder class
*  This is a subclass of Object, and hence implements the
*  methods intersect() and normal().
-------------------------------------------------------------*/

#include "Cylinder.h"
#include <math.h>

/**
* Cylinder's intersection method.  The input is a ray.
*/
float Cylinder::intersect(glm::vec3 p0, glm::vec3 dir)
{
    float a = (dir.x * dir.x + dir.z * dir.z);
    float b = dir.x * (p0.x - center.x) + dir.z * (p0.z - center.z);
    float c = ((p0.x - center.x) * (p0.x - center.x)) + ((p0.z - center.z) * (p0.z - center.z)) - (radius * radius);
    float delta = b * b - a * c;

    if (delta < 0.001 || a == 0) return -1.0;    //includes zero and negative values
    
    float t1 = (-b - sqrt(delta)) / a;
    float t2 = (-b + sqrt(delta)) / a;

    glm::vec3 p = p0 + t1 * dir;
    if (p.y > center.y + height) t1 = -2;
    if (p.y < center.y) t1 = -1;

    p = p0 + t2 * dir;
    if (p.y > center.y + height) { 
        t2 = -1; 
    }
    else if (t1 == -2) {
        t1 = (center.y + height - p0.y)/ dir.y;
    }
    if (p.y < center.y) t2 = -1;

    return t1;
}

/**
* Returns the unit normal vector at a given point.
* Assumption: The input point p lies on the cylinder.
*/
glm::vec3 Cylinder::normal(glm::vec3 p)
{
    glm::vec3 n = (p - center) * glm::vec3(1,0,1);
    n = glm::normalize(n);
    return n;
}
