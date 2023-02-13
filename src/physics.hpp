#pragma once

#include "base.h"

struct Object {
	virtual Vector2 supportFunction(Vector2 direction) const;
};

bool collides(const Object&, const Object&);


