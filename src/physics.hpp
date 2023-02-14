#pragma once

#include "base.h"
#include <vector>

struct Object {
	virtual Vector2 supportFunction(float angle) const = 0;
};

bool collides(const Object&, const Object&);

struct CircObj: public Object {
	Vector2 pos;
	float rad;
	CircObj(Vector2 pos, float rad): pos(pos), rad(rad) {}
	Vector2 supportFunction(float angle) const override;
};

class ObjectHandler {
	std::vector<Object*> objects;
public:
	void addObject(Object*);
	bool collidesWith(const Object&);
};


