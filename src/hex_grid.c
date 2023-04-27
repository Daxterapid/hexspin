#include "hexspin/common.h"
#include "hexspin/hex_grid.h"
#include "hexspin/shader.h"
#include "hexspin/animation.h"

// TODO: make small utility functions for minor repetitive things
// TODO: macro for modulo?

struct hexagon {
	uint32_t home_q;
	uint32_t home_r;
	uint32_t degree;
};

struct hex_grid {
	uint32_t cols;
	uint32_t rows;
	
	uint32_t n_cells;
	int32_t *visible_map;
	uint32_t *reorder_map;
	struct hexagon *cells;
	
	struct hex_grid_bounds bounds;
	
	bool has_selection;
	uint32_t selected_q;
	uint32_t selected_r;
	
	struct event_emitter *emitter;
};

int32_t hex_grid_get_internal_index(struct hex_grid *grid,
		uint32_t q, uint32_t r)
{
	if (hex_grid_get_visible_index(grid, q, r) == -1) {
		return -1;
	}
	
	return grid->reorder_map[grid->visible_map[r * grid->cols + q]];
}

int32_t hex_grid_get_visible_index(struct hex_grid *grid,
		uint32_t q, uint32_t r)
{
	return grid->visible_map[r * grid->cols + q];
}

struct hex_grid *hex_grid_create(uint32_t cols, uint32_t rows, bool *bitmap,
		bool invert)
{
	if (cols < 2 || rows < 2) {
		exit(EXIT_FAILURE); // TODO: actual error handling
	}
	
	struct hex_grid *grid = malloc(sizeof(*grid));
	assert(grid);
	
	grid->cols = cols;
	grid->rows = rows;
	
	grid->visible_map = malloc(sizeof(*grid->visible_map) * cols * rows);
	assert(grid->visible_map);
	
	uint32_t n_cells = 0;
	for (uint32_t i = 0; i < cols * rows; i++) {
		if (bitmap[i] ^ invert) {
			grid->visible_map[i] = n_cells;
			n_cells++;
		} else {
			grid->visible_map[i] = -1;
		}
	}
	grid->n_cells = n_cells;
	
	grid->reorder_map = malloc(sizeof(*grid->reorder_map) * n_cells);
	assert(grid->reorder_map);
	grid->cells = malloc(sizeof(*grid->cells) * n_cells);
	assert(grid->cells);
	
	grid->bounds.x_min = cols - 1 + (rows - 1) * 0.5f;
	grid->bounds.x_max = 0.0f;
	grid->bounds.y_min = rows;
	grid->bounds.y_max = 0.0f;
	
	for (uint32_t i = 0; i < cols * rows; i++) {
		int32_t visible_index = grid->visible_map[i];
		if (visible_index == -1) {
			continue;
		}
		
		uint32_t q = i % cols;
		uint32_t r = i / rows;
		
		float x = q + r * 0.5f;
		if (x < grid->bounds.x_min) {
			grid->bounds.x_min = x;
		}
		if (x > grid->bounds.x_max) {
			grid->bounds.x_max = x;
		}
		if (r < grid->bounds.y_min) {
			grid->bounds.y_min = r;
		}
		if (r > grid->bounds.y_max) {
			grid->bounds.y_max = r;
		}
		
		grid->reorder_map[visible_index] = visible_index;
		
		grid->cells[visible_index].home_q = q;
		grid->cells[visible_index].home_r = r;
		grid->cells[visible_index].degree = 0;
	}
	
	grid->emitter = event_emitter_create();
	
	return grid;
}

void hex_grid_destroy(struct hex_grid *grid)
{
	free(grid->visible_map);
	free(grid->reorder_map);
	free(grid->cells);
	
	event_emitter_destroy(grid->emitter);
	
	free(grid);
}

uint32_t hex_grid_cols(struct hex_grid *grid)
{
	return grid->cols;
}

uint32_t hex_grid_rows(struct hex_grid *grid)
{
	return grid->rows;
}

uint32_t hex_grid_n_cells(struct hex_grid *grid)
{
	return grid->n_cells;
}

bool hex_grid_has_selection(struct hex_grid *grid)
{
	return grid->has_selection;
}

uint32_t hex_grid_cell_degree(struct hex_grid *grid, uint32_t q, uint32_t r)
{
	uint32_t i = hex_grid_get_internal_index(grid, q, r);
	
	return grid->cells[i].degree;
}

struct hex_grid_bounds hex_grid_get_bounds(struct hex_grid *grid)
{
	return grid->bounds;
}

void hex_grid_event_on(struct hex_grid *grid, enum hex_grid_event event,
		event_emitter_callback_pfn callback, void *data)
{
	event_emitter_on(grid->emitter, event, callback, data);
}

int randint(int max)
{
	return rand() / (RAND_MAX / max + 1);
}

void hex_grid_scramble(struct hex_grid *grid)
{
	for (uint32_t i = grid->n_cells - 1; i > 0; i--) {
		uint32_t j = randint(i);
		
		uint32_t end_copy = grid->reorder_map[i];
		grid->reorder_map[i] = grid->reorder_map[j];
		grid->reorder_map[j] = end_copy;
	}
	
	for (uint32_t r = 0; r < grid->rows; r++) {
		for (uint32_t q = 0; q < grid->cols; q++) {
			int32_t i = hex_grid_get_internal_index(grid, q, r);
			if (i == -1) {
				continue;
			}
			
			int32_t rel_q = q - grid->cells[i].home_q;
			int32_t rel_r = r - grid->cells[i].home_r;
			grid->cells[i].degree = (((rel_q - rel_r) % 3) + 3) % 3;
		}
	}
	
	event_emitter_emit(grid->emitter, HEX_GRID_EVENT_SCRAMBLE, NULL);
}

bool valid_selection(struct hex_grid *grid, uint32_t q, uint32_t r)
{
	if (r == 0 || q + 1 >= grid->cols) {
		return false;
	}
	
	uint32_t i = r * grid->cols + q;
	uint32_t tl_i = i - grid->cols;
	uint32_t tr_i = tl_i + 1;
	
	return grid->visible_map[i] != -1 &&
		grid->visible_map[tl_i] != -1 &&
		grid->visible_map[tr_i] != -1;
}

bool hex_grid_select(struct hex_grid *grid, uint32_t q, uint32_t r)
{
	if (q >= grid->cols || r >= grid->rows) {
		return false;
	}
	
	if (valid_selection(grid, q, r)) {
		grid->has_selection = true;
		grid->selected_q = q;
		grid->selected_r = r;
		
		struct hex_grid_event_params_select params = {
			grid->selected_q,
			grid->selected_r
		};
		event_emitter_emit(grid->emitter, HEX_GRID_EVENT_SELECT, &params);
		
		return true;
	}
	
	return false;
}

void hex_grid_deselect(struct hex_grid *grid)
{
	grid->has_selection = false;
}

void hex_grid_select_nq(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (hex_grid_select(grid, q - 1, r)) return;
	if (hex_grid_select(grid, q - 1, r + 1)) return;
	hex_grid_select(grid, q, r - 1);
}

void hex_grid_select_q(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (hex_grid_select(grid, q + 1, r)) return;
	if (hex_grid_select(grid, q, r + 1)) return;
	hex_grid_select(grid, q + 1, r - 1);
}

void hex_grid_select_nr(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (!hex_grid_select(grid, q, r - 1)) {
		hex_grid_select(grid, q + 1, r - 1);
	}
}

void hex_grid_select_r(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (!hex_grid_select(grid, q, r + 1)) {
		hex_grid_select(grid, q - 1, r + 1);
	}
}

void hex_grid_select_nqr(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (!hex_grid_select(grid, q, r + 1)) {
		hex_grid_select(grid, q - 1, r + 1);
	}
}

void hex_grid_select_qr(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	if (!hex_grid_select(grid, q + 1, r - 1)) {
		hex_grid_select(grid, q - 1, r - 1);
	}
}

void hex_grid_rotate_cw(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	uint32_t b_i = hex_grid_get_internal_index(grid, q, r);
	uint32_t tl_i = hex_grid_get_internal_index(grid, q, r - 1);
	uint32_t tr_i = hex_grid_get_internal_index(grid, q + 1, r - 1);
	
	grid->reorder_map[hex_grid_get_visible_index(grid, q, r)] = tr_i;
	grid->reorder_map[hex_grid_get_visible_index(grid, q + 1, r - 1)] = tl_i;
	grid->reorder_map[hex_grid_get_visible_index(grid, q, r - 1)] = b_i;
	
	grid->cells[b_i].degree = (grid->cells[b_i].degree + 1) % 3;
	grid->cells[tl_i].degree = (grid->cells[tl_i].degree + 1) % 3;
	grid->cells[tr_i].degree = (grid->cells[tr_i].degree + 1) % 3;
	
	struct hex_grid_event_params_rotate params = {
		q, r,
		q, r - 1,
		q + 1, r - 1
	};
	event_emitter_emit(grid->emitter, HEX_GRID_EVENT_ROTATE, &params);
}

void hex_grid_rotate_ccw(struct hex_grid *grid)
{
	uint32_t q = grid->selected_q;
	uint32_t r = grid->selected_r;
	
	uint32_t b_i = hex_grid_get_internal_index(grid, q, r);
	uint32_t tl_i = hex_grid_get_internal_index(grid, q, r - 1);
	uint32_t tr_i = hex_grid_get_internal_index(grid, q + 1, r - 1);
	
	grid->reorder_map[hex_grid_get_visible_index(grid, q, r)] = tl_i;
	grid->reorder_map[hex_grid_get_visible_index(grid, q, r - 1)] = tr_i;
	grid->reorder_map[hex_grid_get_visible_index(grid, q + 1, r - 1)] = b_i;
	
	grid->cells[b_i].degree = (grid->cells[b_i].degree + 2) % 3;
	grid->cells[tl_i].degree = (grid->cells[tl_i].degree + 2) % 3;
	grid->cells[tr_i].degree = (grid->cells[tr_i].degree + 2) % 3;
	
	struct hex_grid_event_params_rotate params = {
		q, r,
		q, r - 1,
		q + 1, r - 1
	};
	event_emitter_emit(grid->emitter, HEX_GRID_EVENT_ROTATE, &params);
}
