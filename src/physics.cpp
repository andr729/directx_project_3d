#include "physics.hpp"
#include "mazephysics.hpp"
#include <utility>
#include <cassert>


float dot(Vector2 v1, Vector2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

struct NearestSimplex {
	Vector2 p0, p1;
	Vector2 direction;
};

bool collides(const Object& p, const Object& q) {
	assert(q.type() == ObjectType::Hex);
	const HexObj& qh = dynamic_cast<const HexObj&>(q);

	if (p.type() == ObjectType::Hex) {
		const HexObj& ph = dynamic_cast<const HexObj&>(p);

		float r = ph.width + qh.width;

		return (ph.t.translation - qh.t.translation).abs2() <= r*r;
	}

	if (p.type() == ObjectType::Rect) {
		const RectangleObj& pr = dynamic_cast<const RectangleObj&>(p);

		Vector2 rect = (pr.t.translation - qh.t.translation).rot(pr.t.rotation);
		Vector2 circle = {0, 0};

		float rect_h = pr.width;
		float rect_w = pr.length;
		float c_r = qh.width;

		Vector2 circleDistance = {std::abs(circle.x - rect.x), std::abs(circle.y - rect.y)};

		if (circleDistance.x > (rect_w/2 + c_r)) { return false; }
		if (circleDistance.y > (rect_h/2 + c_r)) { return false; }

		if (circleDistance.x <= (rect_w/2)) { return true; } 
		if (circleDistance.y <= (rect_h/2)) { return true; }

		float au1 = (circleDistance.x - rect_w/2);
		float au2 = (circleDistance.y - rect_h/2);
		float cornerDistance_sq = au1*au1 + au2*au2;
		return (cornerDistance_sq <= (c_r * c_r));
	}

	return false;
}


void ObjectHandler::addObject(Object* obj) {
	objects.push_back(obj);
}

bool ObjectHandler::collidesWith(const Object& obj) const {
	for (const auto& o: objects) {
		if (collides(*o, obj)) {
			return true;
		}
	}
	return false;
}
