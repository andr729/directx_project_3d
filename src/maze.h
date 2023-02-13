#pragma once
#include <vector>
#include "base.h"

constexpr int CUBOID_VERTEX_COUNT = 3 * 6 * 2;
constexpr int HEXPRISM_VERTEX_COUNT = 3 * (2 * 4 + 6 * 2);

struct CuboidTransformation {
	Vector2 translation;
	float rotation;
};

struct HexPrismTransformation {
	Vector2 translation;
};

struct Maze {
	vertex_t cuboid[CUBOID_VERTEX_COUNT];
	vertex_t hexprism[HEXPRISM_VERTEX_COUNT];
	std::vector<CuboidTransformation> transformations_cuboid;
	std::vector<HexPrismTransformation> transformations_hexprism;
};

Maze getMaze(float length, float width, float height, int side_edges, int seed = 14369);
