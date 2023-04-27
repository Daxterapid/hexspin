#ifndef HEX_GRID_H
#define HEX_GRID_H

#include "hexspin/common.h"
#include "hexspin/event.h"

enum hex_grid_event {
	HEX_GRID_EVENT_SCRAMBLE,
	HEX_GRID_EVENT_ROTATE,
	HEX_GRID_EVENT_SELECT
};

struct hex_grid_event_params_rotate {
	uint32_t b_q;
	uint32_t b_r;
	uint32_t tl_q;
	uint32_t tl_r;
	uint32_t tr_q;
	uint32_t tr_r;
};

struct hex_grid_event_params_select {
	uint32_t q;
	uint32_t r;
};

struct hex_grid_bounds {
	float x_min;
	float x_max;
	float y_min;
	float y_max;
};

struct hex_grid;

struct hex_grid *hex_grid_create(uint32_t width, uint32_t height, bool *bitmap,
		bool invert);
void hex_grid_destroy(struct hex_grid *grid);

int32_t hex_grid_get_internal_index(struct hex_grid *grid,
		uint32_t q, uint32_t r);
int32_t hex_grid_get_visible_index(struct hex_grid *grid,
		uint32_t q, uint32_t r);

uint32_t hex_grid_cols(struct hex_grid *grid);
uint32_t hex_grid_rows(struct hex_grid *grid);
uint32_t hex_grid_n_cells(struct hex_grid *grid);
bool hex_grid_has_selection(struct hex_grid *grid);
uint32_t hex_grid_cell_degree(struct hex_grid *grid, uint32_t q, uint32_t r);
struct hex_grid_bounds hex_grid_get_bounds(struct hex_grid *grid);

void hex_grid_event_on(struct hex_grid *grid, enum hex_grid_event,
		event_emitter_callback_pfn callback, void *data);

void hex_grid_scramble(struct hex_grid *grid);

bool hex_grid_select(struct hex_grid *grid, uint32_t q, uint32_t r);
void hex_grid_deselect(struct hex_grid *grid);
void hex_grid_select_nq(struct hex_grid *grid);
void hex_grid_select_q(struct hex_grid *grid);
void hex_grid_select_nr(struct hex_grid *grid);
void hex_grid_select_r(struct hex_grid *grid);
void hex_grid_select_nqr(struct hex_grid *grid);
void hex_grid_select_qr(struct hex_grid *grid);

void hex_grid_rotate_cw(struct hex_grid *grid);
void hex_grid_rotate_ccw(struct hex_grid *grid);

#endif
