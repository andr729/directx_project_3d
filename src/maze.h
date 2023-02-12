#pragma once
#include <vector>
#include "base.h"


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

Maze getMaze(float length, float width, float height, int side_edges, int seed = 14369);
