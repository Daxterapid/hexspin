#include "hexspin/common.h"
#include "hexspin/hex_grid.h"
#include "hexspin/shader.h"
#include "hexspin/hex_grid_renderer.h"

void scramble_listener(void *params, void *data);
void rotate_listener(void *params, void *data);
void select_listener(void *params, void *data);

void transform_to_mat4(struct transform *transform, float matrix[16])
{
	// In column-major order
	matrix[0] = transform->scale.x * cosf(transform->rotation);
	matrix[4] = transform->scale.y * -sinf(transform->rotation);
	matrix[8] = 0.0f;
	matrix[12] = transform->position.x;
	matrix[1] = transform->scale.x * sinf(transform->rotation);
	matrix[5] = transform->scale.y * cosf(transform->rotation);
	matrix[9] = 0.0f;
	matrix[13] = transform->position.y;
	matrix[2] = 0.0f;
	matrix[6] = 0.0f;
	matrix[10] = transform->scale.z;
	matrix[14] = transform->position.z;
	matrix[3] = 0.0f;
	matrix[7] = 0.0f;
	matrix[11] = 0.0f;
	matrix[15] = 1.0f;
}

float map(float x, float in_start, float in_end, float out_start, float out_end)
{
	float slope = (out_end - out_start) / (in_end - in_start);
	return out_start + slope * (x - in_start);
}

void hsv2rgb(float h, float s, float v, float *r, float *g, float *b)
{
	float c = v * s;
	float x = c * (1.0f - fabsf(fmodf(h * 6.0f, 2.0f) - 1.0f));
	float m = v - c;
	
	int sector = h * 6;
	float rgb[3];
	rgb[(7 - sector) % 3] = x + m;
	rgb[((sector + 1) / 2) % 3] = c + m;
	rgb[(sector / 2 + 2) % 3] = m;
	
	*r = rgb[0];
	*g = rgb[1];
	*b = rgb[2];
}

void sample_color(float x, float y, float *r, float *g, float *b)
{
	float vx = x;
	float vy = y;
	
	float angle = atan2f(vy, vx);
	angle = angle < 0.0f ? TWO_PI + angle : angle;
	float length = sqrtf(vx * vx + vy * vy);
	
	hsv2rgb(angle / TWO_PI, length * 0.8f, 0.8f, r, g, b);
}

struct hex_grid_renderer {
	struct hex_grid *grid;
	uint32_t cols;
	uint32_t rows;
	uint32_t n_cells;
	
	float *colors;
	float *transforms;
	
	GLuint hex_shader;
	GLuint highlight_shader;
	
	GLuint hex_vao;
	GLuint hex_vertices;
	GLuint hex_transforms;
	GLuint vertex_colors;
	GLuint color_texture;
	
	GLuint highlight_vao;
	GLuint highlight_vbo;
	float highlight_translate[3];
};

struct hex_grid_renderer *hex_grid_renderer_create(struct hex_grid *grid)
{
	struct hex_grid_renderer *renderer = malloc(sizeof(*renderer));
	assert(renderer);
	
	hex_grid_event_on(grid, HEX_GRID_EVENT_SCRAMBLE, scramble_listener, renderer);
	hex_grid_event_on(grid, HEX_GRID_EVENT_ROTATE, rotate_listener, renderer);
	hex_grid_event_on(grid, HEX_GRID_EVENT_SELECT, select_listener, renderer);
	
	renderer->grid = grid;
	renderer->cols = hex_grid_cols(grid);
	renderer->rows = hex_grid_rows(grid);
	renderer->n_cells = hex_grid_n_cells(grid);
	
	renderer->colors = malloc(sizeof(*renderer->colors) * renderer->n_cells * 8 * 3);
	assert(renderer->colors);
	renderer->transforms = malloc(sizeof(*renderer->transforms) * renderer->n_cells * 16);
	assert(renderer->transforms);
	
	float vertices[8 * 3] = { 0.0f };
	for (uint32_t i = 0; i < 7; i++) {
		vertices[(i + 1) * 3 + 0] = cosf(i / 6.0f * TWO_PI + HALF_PI);
		vertices[(i + 1) * 3 + 1] = sinf(i / 6.0f * TWO_PI + HALF_PI);
	}
	
	for (uint32_t vis_i = 0; vis_i < renderer->cols * renderer->rows; vis_i++) {
		uint32_t q = vis_i % renderer->cols;
		uint32_t r = vis_i / renderer->rows;
		
		int32_t i = hex_grid_get_internal_index(grid, q, r);
		if (i == -1) {
			continue;
		}
		
		struct hex_grid_bounds bounds = hex_grid_get_bounds(grid);
		float cx_left = (bounds.x_min) * sqrtf(3.0f);
		float cx_right = (bounds.x_max) * sqrtf(3.0f);
		float cy_top = (bounds.y_min) * 1.5f;
		float cy_bottom = (bounds.y_max) * 1.5f;
		float x_left = (bounds.x_min - 0.5f) * sqrtf(3.0f);
		float x_right = (bounds.x_max + 0.5f) * sqrtf(3.0f);
		float y_top = (bounds.y_min - 0.5f) * 1.5f;
		float y_bottom = (bounds.y_max + 0.5f) * 1.5f;
		
		float x = (q + r * 0.5f) * sqrtf(3.0f);
		float y = r * 1.5f;
		
		for (uint32_t j = 0; j < 8; j++) {
			float norm_x = map(x + vertices[j * 3], x_left, x_right, -1.0f, 1.0f);
			float norm_y = map(y + vertices[j * 3 + 1], y_top, y_bottom, -1.0f, 1.0f);
			
			float *color = &renderer->colors[i * 24 + j * 3];
			sample_color(norm_x, norm_y, color, color + 1, color + 2);
		}
		
		float half_width = (cx_right - cx_left) * 0.5f;
		float half_height = (cy_bottom - cy_top) * 0.5f;
		
		struct transform transform = {
			{
				map(x, cx_left, cx_right, -half_width, half_width),
				map(y, cy_top, cy_bottom, -half_height, half_height),
				0.0f
			},
			{ 1.0f, 1.0f, 1.0f },
			0.0f
		};
		
		transform_to_mat4(&transform, &renderer->transforms[i * 16]);
	}
	
	renderer->hex_shader = create_shaderf("assets/hexagons.vert", "assets/color.frag");
	renderer->highlight_shader = create_shaderf("assets/highlight.vert", "assets/color.frag");
	
	// Shader data
	glGenVertexArrays(1, &renderer->hex_vao);
	glGenBuffers(1, &renderer->hex_vertices);
	glGenBuffers(1, &renderer->hex_transforms);
	glGenBuffers(1, &renderer->vertex_colors);
	glGenTextures(1, &renderer->color_texture);
	
	glBindVertexArray(renderer->hex_vao);
	
	// Vertices
	glBindBuffer(GL_ARRAY_BUFFER, renderer->hex_vertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	// Colors
	glBindBuffer(GL_TEXTURE_BUFFER, renderer->vertex_colors);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(*renderer->colors) * 8 * 3 * renderer->n_cells,
		renderer->colors, GL_STATIC_DRAW);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, renderer->color_texture);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, renderer->vertex_colors);
	glBindBuffer(GL_TEXTURE_BUFFER, 0);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	
	// Transforms
	glBindBuffer(GL_ARRAY_BUFFER, renderer->hex_transforms);
	glBufferData(GL_ARRAY_BUFFER, sizeof(*renderer->transforms) * 16 * renderer->n_cells,
		renderer->transforms, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void *)0);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void *)(4 * sizeof(float)));
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void *)(8 * sizeof(float)));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 16 * sizeof(float), (void *)(12 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glBindVertexArray(0);
	
	// Highlight
	float hl_vertices[26 * 3] = { 0.0f };
	
	// Angles are rotated -90deg
	float angles[13] = {
		1, 1, 4 / 3.0f, 5 / 3.0f, 5 / 3.0f, 5 / 3.0f,
		0, 1 / 3.0f, 1 / 3.0f, 1 / 3.0f, 2 / 3.0F, 1, 1
	};
	
	float x_offsets[] = { 0, 1, 2, 2, 1, 1, 0, -1, -1, -2, -2, -1, 0 };
	float y_offsets[] = { -4, -5, -4, -2, -1, 1, 2, 1, -1, -2, -4, -5, -4 };
	
	float rw = sqrtf(3.0f) * 0.5f;
	float rh = 0.5f;
	
	float in_mag = 0.0f;
	float out_mag = 0.15f;
	
	for (uint32_t i = 0; i < 13; i++) {
		float angle = PI * angles[i] + HALF_PI;
		float x = rw * x_offsets[i];
		float y = rh * y_offsets[i];
		
		float out_x = x + cos(angle) * out_mag;
		float out_y = y + sin(angle) * out_mag;
		float in_x = x + cos(angle) * in_mag;
		float in_y = y + sin(angle) * in_mag;
		
		hl_vertices[i * 6 + 0] = out_x;
		hl_vertices[i * 6 + 1] = out_y;
		hl_vertices[i * 6 + 3] = in_x;
		hl_vertices[i * 6 + 4] = in_y;
	}
	
	glGenVertexArrays(1, &renderer->highlight_vao);
	glGenBuffers(1, &renderer->highlight_vbo);
	
	glBindVertexArray(renderer->highlight_vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer->highlight_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 26, hl_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	glBindVertexArray(0);
	
	renderer->highlight_translate[2] = 0.0f;
	
	return renderer;
}

void hex_grid_renderer_destroy(struct hex_grid_renderer *renderer)
{
	glDeleteTextures(1, &renderer->color_texture);
	
	glDeleteBuffers(1, &renderer->hex_vertices);
	glDeleteBuffers(1, &renderer->hex_transforms);
	glDeleteBuffers(1, &renderer->vertex_colors);
	glDeleteBuffers(1, &renderer->highlight_vbo);
	
	glDeleteVertexArrays(1, &renderer->hex_vao);
	glDeleteVertexArrays(1, &renderer->highlight_vao);
	
	glDeleteProgram(renderer->hex_shader);
	glDeleteProgram(renderer->highlight_shader);
	
	free(renderer->colors);
	free(renderer->transforms);
	free(renderer);
}

void hex_grid_renderer_render(struct hex_grid_renderer *renderer, float width,
		float height, float radius)
{
	float mat_proj[16] = { 0.0f };
	mat_proj[0] = 1.0f / (width * 0.5f);
	mat_proj[5] = -1.0f / (height * 0.5f);
	mat_proj[10] = 1.0f;
	mat_proj[15] = 1.0f;
	mat_proj[3] = -1.0f;
	mat_proj[7] = 1.0f;
	
	float mat_view[16] = { 0.0f };
	struct transform view_transform = {
		{ width * 0.5f, height * 0.5f, 0.0f },
		{ 1.0f, 1.0f, 1.0f },
		0.0f
	};
	transform_to_mat4(&view_transform, mat_view);
	
	glUseProgram(renderer->hex_shader);
	
	GLuint view_loc = glGetUniformLocation(renderer->hex_shader, "uView");
	GLuint proj_loc = glGetUniformLocation(renderer->hex_shader, "uProj");
	GLuint radius_loc = glGetUniformLocation(renderer->hex_shader, "uRadius");
	GLuint tex_loc = glGetUniformLocation(renderer->hex_shader, "uColors");
	
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, mat_view);
	glUniformMatrix4fv(proj_loc, 1, GL_TRUE, mat_proj);
	glUniform1f(radius_loc, radius);
	glUniform1i(tex_loc, 0);
	
	glBindTexture(GL_TEXTURE_BUFFER, renderer->color_texture);
	glBindVertexArray(renderer->hex_vao);
	glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 8, renderer->n_cells);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
	glBindVertexArray(0);
	
	if (hex_grid_has_selection(renderer->grid)) {
		glUseProgram(renderer->highlight_shader);
		
		GLuint h_view_loc = glGetUniformLocation(renderer->highlight_shader, "uView");
		GLuint h_proj_loc = glGetUniformLocation(renderer->highlight_shader, "uProj");
		GLuint h_trans_loc = glGetUniformLocation(renderer->highlight_shader, "uTranslate");
		GLuint h_radius_loc = glGetUniformLocation(renderer->highlight_shader, "uRadius");
		
		glUniformMatrix4fv(h_view_loc, 1, GL_FALSE, mat_view);
		glUniformMatrix4fv(h_proj_loc, 1, GL_TRUE, mat_proj);
		glUniform3fv(h_trans_loc, 1, renderer->highlight_translate);
		glUniform1f(h_radius_loc, radius);
		
		glBindVertexArray(renderer->highlight_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 26);
		glBindVertexArray(0);
	}
}

void scramble_listener(void *params, void *data)
{
	struct hex_grid_renderer *renderer = data;
	
	for (uint32_t vis_i = 0; vis_i < renderer->cols * renderer->rows; vis_i++) {
		uint32_t q = vis_i % renderer->cols;
		uint32_t r = vis_i / renderer->rows;
		
		int32_t i = hex_grid_get_internal_index(renderer->grid, q, r);
		if (i == -1) {
			continue;
		}
		
		struct hex_grid_bounds bounds = hex_grid_get_bounds(renderer->grid);
		float cx_left = (bounds.x_min) * sqrtf(3.0f);
		float cx_right = (bounds.x_max) * sqrtf(3.0f);
		float cy_top = (bounds.y_min) * 1.5f;
		float cy_bottom = (bounds.y_max) * 1.5f;
		
		float x = (q + r * 0.5f) * sqrtf(3.0f);
		float y = r * 1.5f;
		
		float half_width = (cx_right - cx_left) * 0.5f;
		float half_height = (cy_bottom - cy_top) * 0.5f;
		
		struct transform transform = {
			{
				map(x, cx_left, cx_right, -half_width, half_width),
				map(y, cy_top, cy_bottom, -half_height, half_height),
				0.0f
			},
			{ 1.0f, 1.0f, 1.0f },
			0.0f
		};
		
		transform_to_mat4(&transform, &renderer->transforms[i * 16]);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer->hex_transforms);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(*renderer->transforms) * renderer->n_cells * 16,
		renderer->transforms);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void rotate_listener(void *params, void *data)
{
	struct hex_grid_event_params_rotate *rotate_params = params;
	struct hex_grid_renderer *renderer = data;
	
	uint32_t q_arr[] = { rotate_params->b_q, rotate_params->tl_q, rotate_params->tr_q };
	uint32_t r_arr[] = { rotate_params->b_r, rotate_params->tl_r, rotate_params->tr_r };
	uint32_t i_arr[3];
	
	for (uint32_t arr_i = 0; arr_i < 3; arr_i++) {
		uint32_t q = q_arr[arr_i];
		uint32_t r = r_arr[arr_i];
		
		uint32_t degree = hex_grid_cell_degree(renderer->grid, q, r);
		int32_t i = hex_grid_get_internal_index(renderer->grid, q, r);
		i_arr[arr_i] = i;
		
		struct hex_grid_bounds bounds = hex_grid_get_bounds(renderer->grid);
		float cx_left = (bounds.x_min) * sqrtf(3.0f);
		float cx_right = (bounds.x_max) * sqrtf(3.0f);
		float cy_top = (bounds.y_min) * 1.5f;
		float cy_bottom = (bounds.y_max) * 1.5f;
		
		float x = (q + r * 0.5f) * sqrtf(3.0f);
		float y = r * 1.5f;
		
		float half_width = (cx_right - cx_left) * 0.5f;
		float half_height = (cy_bottom - cy_top) * 0.5f;
		
		struct transform transform = {
			{
				map(x, cx_left, cx_right, -half_width, half_width),
				map(y, cy_top, cy_bottom, -half_height, half_height),
				0.0f
			},
			{ 1.0f, 1.0f, 1.0f },
			degree * TWO_PI / 3.0f
		};
		
		transform_to_mat4(&transform, &renderer->transforms[i * 16]);
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, renderer->hex_transforms);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(*renderer->transforms) * 16 * i_arr[0],
		sizeof(*renderer->transforms) * 16, &renderer->transforms[i_arr[0] * 16]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(*renderer->transforms) * 16 * i_arr[1],
		sizeof(*renderer->transforms) * 16, &renderer->transforms[i_arr[1] * 16]);
	glBufferSubData(GL_ARRAY_BUFFER, sizeof(*renderer->transforms) * 16 * i_arr[2],
		sizeof(*renderer->transforms) * 16, &renderer->transforms[i_arr[2] * 16]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void select_listener(void *params, void *data)
{
	struct hex_grid_event_params_select *select_params = params;
	struct hex_grid_renderer *renderer = data;
	
	uint32_t q = select_params->q;
	uint32_t r = select_params->r;
	
	struct hex_grid_bounds bounds = hex_grid_get_bounds(renderer->grid);
	float cx_left = (bounds.x_min) * sqrtf(3.0f);
	float cx_right = (bounds.x_max) * sqrtf(3.0f);
	float cy_top = (bounds.y_min) * 1.5f;
	float cy_bottom = (bounds.y_max) * 1.5f;
	
	float x = (q + r * 0.5f) * sqrtf(3.0f);
	float y = r * 1.5f;
	
	float half_width = (cx_right - cx_left) * 0.5f;
	float half_height = (cy_bottom - cy_top) * 0.5f;
	
	renderer->highlight_translate[0] = map(x, cx_left, cx_right, -half_width, half_width);
	renderer->highlight_translate[1] = map(y, cy_top, cy_bottom, -half_height, half_height);
}
