#pragma once
#include <vector>

#include <d3d12.h> // FLOAT xd

struct vertex_t {
	float position[3];
	float normal_vector[3];
	float color[4];
};

struct Vector2 {
	float x;
	float y;
};

struct SimpleColor {
	FLOAT r, g, b;
};

struct SimpleVertex {
	FLOAT x;
	FLOAT y;
	FLOAT z;
	SimpleColor color;
};

struct Triangle {
	vertex_t t1[3];
};

struct CuboidTransformation {
	Vector2 translation;
	float rotation;
};

struct HexPrismTransformation {
	Vector2 translation;
};

struct Maze {
	vertex_t cuboid[3 * 6 * 2];
	vertex_t hexprism[3 * (2 * 4 + 6 * 2)];
	std::vector<CuboidTransformation> transformations_cuboid;
	std::vector<HexPrismTransformation> transformations_hexprism;
};


constexpr Triangle makeSingleTriangle(SimpleVertex p1, SimpleVertex p2, SimpleVertex p3);
Maze getMaze(float length, float width, float height, int side_points);

