#ifndef ANIMATION_H
#define ANIMATION_H

// Ease
typedef float (*ease_pfn)(float);

float ease_linear(float t);

// Path
struct Path;

struct Path *Path_create(float value);
void Path_destroy(struct Path *path);

void Path_keyframe_insert(struct Path *path, float time, float value,
		ease_pfn ease);

float Path_sample(struct Path *path, float time);
bool Path_out_of_bounds(struct Path *path, float time);

// Animator
//struct animator;

//void animator_create(struct animator *animator);
//void animator_delete(struct animator *animator);

//void animator_add_(struct track *track);

#endif
