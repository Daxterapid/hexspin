#ifndef SHADER_H
#define SHADER_H

#include "hexspin/common.h"

GLuint create_shader(const char * const vertex_code,
		const char * const fragment_code);
GLuint create_shaderf(char *vertex_name, char *fragment_name);

#endif
