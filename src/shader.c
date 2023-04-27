#include "hexspin/shader.h"

// TODO? shader caching?

GLuint create_shader(const char * const vertex_code,
		const char * const fragment_code)
{
	int success;
	char info_log[1024];
	
	GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vertex_code, NULL);
	glCompileShader(vert_shader);
	
	glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vert_shader, 1024, NULL, info_log);
		fprintf(stderr, "Vertex shader error: %s\n", info_log);
	}
	
	GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &fragment_code, NULL);
	glCompileShader(frag_shader);
	
	glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(frag_shader, 1024, NULL, info_log);
		fprintf(stderr, "Fragment shader error: %s\n", info_log);
	}
	
	GLuint program = glCreateProgram();
	glAttachShader(program, vert_shader);
	glAttachShader(program, frag_shader);
	glLinkProgram(program);
	
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(program, 1024, NULL, info_log);
		fprintf(stderr, "Shader linking error: %s\n", info_log);
	}
	
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	
	return program;
}

GLuint create_shaderf(char *vertex_name, char *fragment_name)
{
	FILE *fp;
	size_t size;
	
	fp = fopen(vertex_name, "rb");
	assert(fp);
	
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	char *vertex_code = malloc(size + 1);
	assert(vertex_code);
	assert(fread(vertex_code, 1, size, fp) == size);
	fclose(fp);
	
	vertex_code[size] = '\0';
	
	fp = fopen(fragment_name, "rb");
	assert(fp);
	
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	char *fragment_code = malloc(size + 1);
	assert(fragment_code);
	assert(fread(fragment_code, 1, size, fp) == size);
	fclose(fp);
	
	fragment_code[size] = '\0';
	
	GLuint program = create_shader(vertex_code, fragment_code);
	
	free(vertex_code);
	free(fragment_code);
	
	return program;
}
