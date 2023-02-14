#pragma once

#include "physics.hpp"
#include "maze.h"

struct Rectangle : Object {
	CuboidTransformation t;
	float length;
	float width;
	Vector2 supportFunction(float angle);
};

struct Hex : Object {
	HexPrismTransformation t;
	float width;
	Vector2 supportFunction(float angle);
};