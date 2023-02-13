#include "physics.hpp"
#include <utility>

float dot(Vector2 v1, Vector2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

struct NearestSimplex {
	Vector2 p0, p1;
	Vector2 direction;
};

struct Triangle {
	Vector2 p[3];
	
	// @TODO: might be wrong:
	bool hasOrigin() const {
		float Area = 0.5 *
			(-p[1].y * p[2].x + p[0].y*(-p[1].x + p[2].x) 
			+ p[0].x * (p[1].y - p[2].y) + p[1].x * p[2].y);
		float s = 1/(2*Area) * 
			(p[0].y*p[2].x - p[0].x*p[2].y);
 		float t = 1/(2*Area) * 
			(p[0].x*p[1].y - p[0].y*p[1].x);
		return s > 0 and t > 0 and 1-s-t > 0;
	}
};

// c is on the left of (b-a)
bool isLeft(Vector2 a, Vector2 b, Vector2 c){
     return ((b.x - a.x)*(c.y - a.y) - (b.y - a.y)*(c.x - a.x)) > 0;
}

bool collides(const Object& p, const Object& q) {
	Vector2 initial_axis0 = {1, 0}; // arbitrary
	Vector2 initial_axis1 = {-1, 0}; // arbitrary

	Vector2 p0 = p.supportFunction(initial_axis0) - q.supportFunction(-initial_axis0);
	Vector2 p1 = p.supportFunction(initial_axis1) - q.supportFunction(-initial_axis1);

	Vector2 delta = p1 - p0;
	Vector2 p2;
	Vector2 initial_axis2;

	Vector2 initial_axis2_guess1 = delta.rot90();
	Vector2 initial_axis2_guess2 = delta.rot270();

	Vector2 p2_guess1 = p.supportFunction(initial_axis2_guess1) - q.supportFunction(-initial_axis2_guess1);
	Vector2 p2_guess2 = p.supportFunction(initial_axis2_guess2) - q.supportFunction(-initial_axis2_guess2);

	if (dot(p2_guess1, initial_axis2_guess1) > 0) {
		p2 = p2_guess1;
		initial_axis2 = initial_axis2_guess1;
	}
	else if (dot(p2_guess1, initial_axis2_guess1) > 0) {
		p2 = p2_guess2;
		initial_axis2 = initial_axis2_guess2;

		// @TODO: here or up:
		std::swap(p0, p1);
	}
	else {
		return false;
	}

	Triangle simplex = {p0, p1, p2};
	
	Vector2 prev_new = p2;
	Vector2 prev_p0 = p0;
	Vector2 prev_p1 = p1;
	Vector2 prev_dir = initial_axis2;

	while (true) {
		if (simplex.hasOrigin()) return true;
		
		Vector2 np0, np1, dir;

		// @TODO: or reverse:
		if (isLeft(prev_p0, prev_new, {0,0})) {
			np0 = prev_p0;
			np1 = prev_new;
		}
		else {
			np0 = prev_new;
			np1 = prev_p1;
		}

		auto new_point = p.supportFunction(dir) - q.supportFunction(-dir);

		if (dot(new_point, dir) < 0) return false;

		simplex = {np0, np1, new_point};
		prev_new = new_point;
		prev_p0 = np0;
		prev_p1 = np1;
	}
}