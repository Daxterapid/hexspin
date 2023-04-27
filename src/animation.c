#include "hexspin/common.h"
#include "hexspin/animation.h"

struct keyframe {
	float time;
	float value;
	ease_pfn ease;
};

float ease_linear(float x)
{
	return x;
}

struct Path {
	struct keyframe *keyframes;
	uint32_t kf_count;
	uint32_t kf_capacity;
	float start_value;
};

struct Path *Path_create(float value)
{
	struct Path *path = malloc(sizeof(*path));
	assert(path);
	
	path->kf_count = 0;
	path->kf_capacity = 1;
	path->keyframes = malloc(sizeof(*path->keyframes) * path->kf_capacity);
	assert(path->keyframes);
	
	path->start_value = value;
	
	return path;
}

void Path_destroy(struct Path *path)
{
	free(path->keyframes);
	free(path);
}

void grow_kf_list(struct Path *path)
{
	path->kf_capacity *= 2;
	path->keyframes = realloc(path->keyframes, sizeof(*path->keyframes) * path->kf_capacity);
	assert(path->keyframes);
}

void Path_keyframe_insert(struct Path *path, float time, float value,
		ease_pfn ease)
{
	assert(time >= 0.0f);
	
	if (path->kf_count >= path->kf_capacity) {
		grow_kf_list(path);
	}
	
	uint32_t insert_index = 0;
	
	if (path->kf_count > 0) {
		insert_index = path->kf_count;
		
		for (uint32_t i = 0; i < path->kf_count; i++) {
			if (path->keyframes[i].time > time) {
				insert_index = i;
				break;
			}
		}
	}
	
	if (insert_index < path->kf_count) {
		memmove(&path->keyframes[insert_index + 1],
			&path->keyframes[insert_index],
			sizeof(*path->keyframes) * (path->kf_count - insert_index));
	}
	
	path->keyframes[insert_index] = (struct keyframe){ time, value, ease };
	
	path->kf_count++;
}

float Path_sample(struct Path *path, float time)
{
	if (path->kf_count == 0 || time < 0.0f) {
		return path->start_value;
	}
	
	uint32_t kf_index = 0;
	
	if (path->kf_count > 0) {
		kf_index = path->kf_count;
		
		for (uint32_t i = 0; i < path->kf_count; i++) {
			if (path->keyframes[i].time > time) {
				kf_index = i;
				break;
			}
		}
	}
	
	if (kf_index == path->kf_count) {
		return path->keyframes[path->kf_count - 1].value;
	}
	
	float lvalue;
	float ltime;
	float rvalue = path->keyframes[kf_index].value;
	float rtime = path->keyframes[kf_index].time;
	
	if (kf_index == 0) {
		lvalue = path->start_value;
		ltime = 0.0f;
	} else {
		lvalue = path->keyframes[kf_index - 1].value;
		ltime = path->keyframes[kf_index - 1].time;
	}
	
	//printf("%d %f->%f, %f->%f\n", kf_index, ltime, rtime, lvalue, rvalue);
	
	float x = (time - ltime) / (rtime - ltime);
	float t = path->keyframes[kf_index].ease(x);
	float interpolated = (1 - t) * lvalue + t * rvalue;
	
	return interpolated;
}

bool Path_out_of_bounds(struct Path *path, float time)
{
	return time < 0.0f || path->kf_count == 0
		|| time > path->keyframes[path->kf_count - 1].time;
}
