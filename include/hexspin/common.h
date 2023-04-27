#ifndef COMMON_H
#define COMMON_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PI 3.14159265f
#define TWO_PI (PI * 2.0f)
#define HALF_PI (PI * 0.5f)

struct vec3 {
	float x;
	float y;
	float z;
};

struct transform {
	struct vec3 position;
	struct vec3 scale;
	float rotation;
};

#endif
