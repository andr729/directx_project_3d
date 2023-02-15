#pragma once

#include "physics.hpp"
#include "maze.hpp"

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
	TranslationTransformation t;
	float width;
	HexObj(TranslationTransformation t, float width):
		t(t), width(width) {}

	Vector2 supportFunction(float angle) const override;
	ObjectType type() const override { return ObjectType::Hex; };
};


