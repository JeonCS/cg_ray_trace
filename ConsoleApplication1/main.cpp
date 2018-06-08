#include <stdlib.h>
#include <algorithm>

#include "Vector.h"
#include "Mesh.h"
#include "Init.h"

extern float light_pos[];
 
Mesh mesh;
GLdouble mvMatrix[16]; // 4 x 4 model view matrix
GLdouble projMatrix[16]; // 4 x 4 projection matrix
GLint viewport[4]; //viewport matrix [0, 0, width, height}

VECTOR3D eye(0.0, 5.0, 5.0);

int mouse_x;
int mouse_y;

struct Ray
{
	// ray
	VECTOR3D origin;
	VECTOR3D dir;
	
	VECTOR3D position(float min_t) 
	{
		VECTOR3D point = min_t * dir + origin;
		return point;
	}

	Ray(VECTOR3D origin, VECTOR3D dir) : origin(origin), dir (dir){}
};

class Object {
public:
	string name;

	VECTOR3D k_ambient;
	VECTOR3D k_diffuse;
	VECTOR3D k_specular;
	float k_shineness;

	Object() {}
	virtual ~Object(){}

	virtual bool hit(Ray r, float *t) = 0 {}
	virtual VECTOR3D getColor(VECTOR3D point, VECTOR3D light, VECTOR3D ray) = 0 {}
	virtual VECTOR3D getNormal(VECTOR3D point) = 0 {}
	virtual VECTOR3D getReflection(VECTOR3D incident, VECTOR3D normal) 
	{
		return incident + (2.0 * normal.InnerProduct(incident) * normal);
	}
	virtual VECTOR3D getRefraction(float n1, float n2, VECTOR3D incident, VECTOR3D normal)
	{
		float n = n1 / n2;
		float cosI = normal.InnerProduct(incident);
		float sinT2 = n * n * (1.0 * cosI * cosI);
		float cosT = sqrt(1.0 - sinT2);
		return n * incident + (n * cosI * cosT) * normal;
	};
};

class Sphere : public Object {
public:
	VECTOR3D cen;
	float rad;

	VECTOR3D k_ambient;
	VECTOR3D k_diffuse;
	VECTOR3D k_specular;
	float k_shineness;

	Sphere() {}

	virtual bool hit(Ray r, float *t) {
		float b = 2.0 * (r.dir.InnerProduct(r.origin - cen));
		float c = pow((r.origin - cen).Magnitude(), 2.0) - (rad * rad);
		float d = (b * b) - (4.0 * c);
		if (d < 0.0)
		{
			*t = 100000;
			return false;
		}

		else if (d == 0)
		{	
			*t = (-1.0 * b / 2.0);
			(*t > 0.01) ? true : false;
		}
		else if (d > 0.0) 
		{
			float t1 = ((-1.0 * b + sqrt(d)) / 2.0);
			float t2 = ((-1.0 * b - sqrt(d)) / 2.0);
			if (t1 > 0.01 && t2 > 0.01)
			{
				*t = (t1 < t2) ? t1 : t2;
				return true;
			}
			else
				return false;	
		}
	}
	virtual VECTOR3D getColor(VECTOR3D point, VECTOR3D light, VECTOR3D ray_origin) 
	{
		VECTOR3D normal = getNormal(point); // normal vector
		normal.Normalize();

		VECTOR3D ray = ray_origin - point; // ray vector
		ray.Normalize();

		VECTOR3D incident = light - point; // incident vector
		incident.Normalize();

		VECTOR3D reflection = (-1.0 * incident) + 2.0 * (incident.InnerProduct(normal) * normal); // reflection vector

		float diffuse = std::max((float) 0.0, incident.InnerProduct(normal));
		float specular = pow(std::max((float) 0.0, ray.InnerProduct(reflection)), k_shineness);
		return diffuse * k_diffuse + specular * k_specular;
		
	}
	virtual VECTOR3D getNormal(VECTOR3D point)
	{
		return point - cen;
	}
};

vector<Sphere*> objects;

VECTOR3D raytrace(Ray ray, int depth) {
	float min_t = 100000;
	Object *hit_obj = nullptr;
	ray.dir.Normalize();

	for (int i = 0; i < objects.size(); i++)
	{
		float t;
		Object *obj = objects[i];
		if (obj->hit(ray, &t))
		{
			if (t <= min_t)
			{
				min_t = t;
				hit_obj = obj;
			}
		}
	}
	if (min_t == 100000)
	{
		// 아무것도 안만났을경우 배경색 return
		return VECTOR3D(0.0, 0.0, 1.0);
	}
	float shadow = 1.0;
	float min_st = 100000;

	VECTOR3D point = ray.position(min_t);
	VECTOR3D normal = hit_obj->getNormal(point);
	VECTOR3D light(light_pos[0], light_pos[1], light_pos[2]);
	VECTOR3D L = light - point;
	normal.Normalize();
	L.Normalize();

	Ray shadow_ray(point, L);

	for (int i = 0; i < objects.size(); i++)
	{
		float t;
		Object *obj = objects[i];
		if (obj->hit(shadow_ray, &t))
		{
			if (t <= min_st) {
				min_st = t;
			}
		}
	}
	if (min_st == 100000)
	{
		shadow = 1.0;
	}
	else 
	{
		shadow = 0.5;
	}

	VECTOR3D reflection = hit_obj->getReflection(point, normal);
	VECTOR3D refraction = hit_obj->getRefraction(1.0, 1.33, point, normal);

	if (depth > 0)
	{
		return hit_obj->k_ambient + shadow * (hit_obj->getColor(point, light, ray.origin))
			+ 0.3 * raytrace(Ray(point, reflection), depth - 1)
			+ 0.3 * raytrace(Ray(point, refraction), depth - 1);
	}
	else
		return hit_obj->k_ambient + shadow * hit_obj->getColor(point, light, ray.origin);
}


void rayCast(int x, int y){

	// To do :: Ray Casting / Intersection Test / Find Intersection Point
	// 1. 입력받은 x, y에 대해 좌표계를 Image에서 Geometry 좌표계로 변환
	// 2. 시작 위치의 vector : origin
	// Ray의 방향을 나타내는 unit vector : v
	// v 방향으로 무한히 먼 위치로 진행하는 vector : v_Infinite
	// 3. (2)과정에서 생성한 3개의 vector를 이용하여 Intersection Test 시행
	// :: Ray-Plane / Ray-Sphere / Ray-Triangle etc…
	// 4. (3)과정에서 Intersection Test가 TRUE일 경우,
	// :: 그 때의 교점의 위치 계산
	glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);

	glBegin(GL_POINTS);
	for (size_t i = 0; i < glutGet(GLUT_WINDOW_WIDTH); i++)
	{
		for (size_t j = 0;  j < glutGet(GLUT_WINDOW_HEIGHT);  j++)
		{
			GLdouble wx, wy, wz;
			// glReadPixels((int)winX, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &zCursor);
			gluUnProject(i, j, 0.0, mvMatrix, projMatrix, viewport, &wx, &wy, &wz);
			VECTOR3D near((float)wx, (float)wy, (float)wz);
			VECTOR3D dir = near - eye;
			Ray ray(eye, dir);
			VECTOR3D color = raytrace(ray, 0);
			glColor3f(color.x, color.y, color.z);
			glVertex3f(near.x, near.y, near.z);
		}
	}
	glEnd();

	// 시작점
}


void meshLoad(void) {

	cout << "loading mesh" << endl;
	mesh.LoadMesh("sphere.obj");
}

void computeNormal(void) {
	cout << "computing face normal" << endl;
	mesh.ComputeFaceNormal();
	cout << "finding neighbor faces" << endl;
	mesh.FindNeighborFaceArray();
	cout << "compute vertex normal" << endl;
	mesh.ComputeVertexNormal();

}

void renderPlane(void) {
	float y = -2.0f;
	glBegin(GL_QUADS);
	glColor3f(0.0f, 1.0f, 0.0f);
	glNormal3f(0, 1, 0);
	glVertex3f(10.0f, y, 10.0f);
	glVertex3f(-10.0f, y, 10.0f);
	glVertex3f(-10.0f, y, -10.0f);
	glVertex3f(10.0f, y, -10.0f);
	glEnd();
}

void renderMesh(void) {
	glBegin(GL_TRIANGLES);
	for (size_t i = 0; i < mesh.faceArray.size(); i++)
	{

		Vertex v0 = mesh.vertexArray[mesh.faceArray[i].vertex0];
		Vertex v1 = mesh.vertexArray[mesh.faceArray[i].vertex1];
		Vertex v2 = mesh.vertexArray[mesh.faceArray[i].vertex2];
	
		glNormal3f(v0.normal.x, v0.normal.y, v0.normal.z);
		glVertex3f(v0.position.x, v0.position.y, v0.position.z);
		glNormal3f(v1.normal.x, v1.normal.y, v1.normal.z);
		glVertex3f(v1.position.x, v1.position.y, v1.position.z);
		glNormal3f(v2.normal.x, v2.normal.y, v2.normal.z);
		glVertex3f(v2.position.x, v2.position.y, v2.position.z);

	}
	glEnd();
}


void redisplayFunc(void)
{
	glMatrixMode(GL_MODELVIEW);
	// clear the drawing buffer.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// clear the identity matrix.
	glLoadIdentity();
	gluLookAt(0.0, 5.0, 5.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	// Red color used to draw.
	renderPlane();

	glPushMatrix();
	glTranslatef(2.0, 0.0, 0.0);
	glColor3f(0.0, 1.0, 0.0);
	renderMesh();
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-2.0, 0.0, 0.0);
	glColor3f(0.0, 0.0, 1.0);
	renderMesh();
	glPopMatrix();

	rayCast(mouse_x, mouse_y);
	glutSwapBuffers();
 
}
 
void reshapeFunc(int x, int y)
{
	glViewport(0, 0, x, y);  //Use the whole window for rendering

	//if (y == 0 || x == 0) return;  //Nothing is visible then, so return
	//Set a new projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
 
	gluPerspective(60.0, (GLdouble)x / (GLdouble)y, 0.1, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}
 
void idleFunc(void)
{
 
 
	redisplayFunc();
}
 
void mouse(int button, int state, int x, int y)
{
	if (button == GLUT_LEFT_BUTTON) {
		if (state == GLUT_UP)
		{
			mouse_x = x;
			mouse_y = y;
			cout << "mouse x:" << mouse_x << endl;
			cout << "mouse y:" << mouse_y << endl;
			glutPostRedisplay();
		}
	}

}
 
int main(int argc, char **argv)
{
	Initialize(argc, argv);
	meshLoad();
	computeNormal();

	// emerald sphere
	Sphere *sph1 = new Sphere();
	sph1->name = "sph1";
	sph1->cen = VECTOR3D( 2.0, 0.0, 0.0 );
	sph1->rad = 1.0;
	sph1->k_ambient = VECTOR3D(0.0215f, 0.1745f, 0.0215f );
	sph1->k_diffuse = VECTOR3D(0.07568, 0.61424, 0.07568);
	sph1->k_specular = VECTOR3D( 0.633, 0.727811, 0.633);
	sph1->k_shineness = 0.6;

	objects.push_back(sph1);


	// copper sphere
	Sphere *sph2 = new Sphere();
	sph2->name = "sph2";
	sph2->cen = VECTOR3D( -2.0, 0.0, 0.0 );
	sph2->rad = 1.0;
	sph2->k_ambient = VECTOR3D(0.19125, 0.0735, 0.0225);
	sph2->k_diffuse = VECTOR3D(0.7038, 0.27048, 0.0828);
	sph2->k_specular = VECTOR3D(0.256777, 0.137622, 0.086014);
	sph2->k_shineness = 0.1;

	objects.push_back(sph2);


	glutDisplayFunc(redisplayFunc);
	glutMouseFunc(mouse);
	glutReshapeFunc(reshapeFunc);
 
	//Let start glut loop
	glutMainLoop();
	return 0;
}
