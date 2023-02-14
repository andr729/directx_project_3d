#include "mazephysics.hpp"

constexpr double PI = 3.14159265358979323846;

Vector2 RectangleObj::supportFunction(float angle) const {
	float rotation = fmod(angle - t.rotation + 2 * PI, 2 * PI);
	if (rotation < PI / 2) {
		return Vector2{length / 2, width / 2} + t.translation;
	}
	if (rotation < PI) {
		return Vector2{length / 2, -width / 2} + t.translation;
	}
	if (rotation < 3 * PI / 2) {
		return Vector2{-length / 2, -width / 2} + t.translation;
	}
	return Vector2{-length / 2, width / 2} + t.translation;
}

Vector2 HexObj::supportFunction(float angle) const {
	if (angle < PI / 3) {
		return Vector2{0, width} + t.translation;
	}
	if (angle < 2 * PI / 3) {
		return Vector2{sqrtf(3) * width, width / 2} + t.translation;
	}
	if (angle < 3 * PI / 3) {
		return Vector2{sqrtf(3) * width, -width / 2} + t.translation;
	}
	if (angle < 4 * PI / 3) {
		return Vector2{0, -width} + t.translation;
	}
	if (angle < 5 * PI / 3) {
		return Vector2{-sqrtf(3) * width, -width / 2} + t.translation;
	}
	return Vector2{-sqrtf(3) * width, width / 2} + t.translation;
}
