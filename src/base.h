#pragma once


struct vertex_t {
	// @TODO: FLOAT
	float position[3];
	float normal_vector[3];
	float color[4];
};

struct Vector2 {
	float x;
	float y;
};

// @TODO: FLOAT
constexpr size_t VERTEX_SIZE = sizeof(vertex_t) / sizeof(float);
