#include "hexspin/common.h"
#include "hexspin/hex_grid.h"
#include "hexspin/hex_grid_renderer.h"
#include "hexspin/shader.h"

// TODO
// - animations
// - mouse selection
// - stop using asserts
// - safe memory allocation wrappers
// - pulsating highlight
// - opposite up/down movement?
// - textures instead of colors
// - sanity checks
// - color outline
// - show axis lines for movement
// - shadow
// - new grid types
// - custom selections and behavior
// - gui
// - audio

/*
struct level {
};

struct level_create_data {
	uint32_t width;
	uint32_t height;
	bool *bitmap;
};

void level_create(struct level *level, struct level_create_data *data)
{

}*/

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	struct hex_grid *grid = glfwGetWindowUserPointer(window);
	if (grid == NULL) return;

	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
		case GLFW_KEY_A:
			hex_grid_select_nq(grid);
			break;
		case GLFW_KEY_D:
			hex_grid_select_q(grid);
			break;
		case GLFW_KEY_W:
			hex_grid_select_nr(grid);
			break;
		case GLFW_KEY_S:
			hex_grid_select_r(grid);
			break;
		}
	}

	if (key == GLFW_KEY_SEMICOLON && action == GLFW_PRESS) {
		hex_grid_rotate_ccw(grid);
	}

	if (key == GLFW_KEY_APOSTROPHE && action == GLFW_PRESS) {
		hex_grid_rotate_cw(grid);
	}

	if (key == GLFW_KEY_I && action == GLFW_PRESS) {
		hex_grid_scramble(grid);
	}
}

void hexagon_bitmap(bool *bitmap, uint32_t size)
{
	for (uint32_t i = 0; i < size / 2; i++) {
		for (uint32_t j = 0; j < size / 2 - i; j++) {
			bitmap[i * size + j] = true;
			bitmap[size * size - (i * size + j) - 1] = true;
		}
	}
}

int main(void)
{
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(750, 650, "hexspin", NULL, NULL);
	if (!window) {
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		exit(EXIT_FAILURE);
	}

#define SIZE 7
	bool bitmap[SIZE * SIZE] = { false };
	hexagon_bitmap(bitmap, SIZE);
	struct hex_grid *grid = hex_grid_create(SIZE, SIZE, bitmap, true);
	struct hex_grid_renderer *renderer = hex_grid_renderer_create(grid);

	glfwSetWindowUserPointer(window, grid);

	srand(time(NULL));
	hex_grid_scramble(grid);

	hex_grid_select(grid, SIZE / 2, SIZE / 2);

	while (!glfwWindowShouldClose(window)) {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//hex_grid_renderer_update(grid, 0);
		hex_grid_renderer_render(renderer, 750.0f, 650.0f, 40.0f);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	hex_grid_destroy(grid);
	hex_grid_renderer_destroy(renderer);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
