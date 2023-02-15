#pragma once

#include "base.hpp"
#include <vector>

enum class ObjectType {
	Rect, Hex
};

struct Object {
	virtual ObjectType type() const = 0;
	virtual Vector2 supportFunction(float angle) const = 0;
};

bool collides(const Object&, const Object&);

class ObjectHandler {
	std::vector<Object*> objects;
public:
	void addObject(Object*);
	bool collidesWith(const Object&) const;
};


