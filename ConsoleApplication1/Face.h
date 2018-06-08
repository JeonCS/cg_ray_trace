#pragma once

#include "VECTOR.h"

class Face
{

public:
	int vertex0;
	int vertex1;
	int vertex2;

	int texture0;
	int texture1;
	int texture2;

	VECTOR3D normal;
};