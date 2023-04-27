#ifndef HEX_GRID_RENDERER_H
#define HEX_GRID_RENDERER_H

#include "hexspin/common.h"
#include "hexspin/hex_grid.h"

struct hex_grid_renderer;

struct hex_grid_renderer *hex_grid_renderer_create(struct hex_grid *grid);
void hex_grid_renderer_destroy(struct hex_grid_renderer *renderer);

//void hex_grid_renderer_update(struct hex_grid_renderer *renderer, float dt);
void hex_grid_renderer_render(struct hex_grid_renderer *renderer, float width,
		float height, float radius);

#endif
