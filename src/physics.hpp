#pragma once

#include "base.h"

struct Object {
	virtual Vector2 supportFunction(float angle) const;
};

bool collides(const Object&, const Object&);


