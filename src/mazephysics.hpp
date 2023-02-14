#pragma once

#include "physics.hpp"
#include "maze.h"

struct RectangleObj: public Object {
	CuboidTransformation t;
	float length;
	float width;
	RectangleObj(CuboidTransformation t, float length, float width):
		t(t), length(length), width(width) {}
	
	Vector2 supportFunction(float angle) const override;

	ObjectType type() const override { return ObjectType::Rect; };
};

struct HexObj: public Object {
	HexPrismTransformation t;
	float width;
	HexObj(HexPrismTransformation t, float width):
		t(t), width(width) {}

	Vector2 supportFunction(float angle) const override;
	ObjectType type() const override { return ObjectType::Hex; };
};


