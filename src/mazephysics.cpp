#include "mazephysics.hpp"

constexpr double PI = 3.14159265358979323846;

Vector2 Rectangle::supportFunction(float angle) {
	float rotation = fmod(angle - t.rotation + 2 * PI, 2 * PI);
	if (rotation < PI / 2) {
		return {length, width} + t.translation;
	}
	if (rotation < PI) {
		return {length, -width} + t.translation;
	}
	if (rotation < 3 * PI / 2) {
		return {-length, -width} + t.translation;
	}
	return {-length, width} + t.translation;
}

Vector2 Hex::supportFunction(float angle) {
	if (rotation < PI / 3) {
		return {0, width} + t.translation;
	}
	if (rotation < 2 * PI / 3) {
		return {sqrt(3) * width, width / 2} + t.translation;
	}
	if (rotation < 3 * PI / 3) {
		return {sqrt(3) * width, -width / 2} + t.translation;
	}
	if (rotation < 4 * PI / 3) {
		return {0, -width} + t.translation;
	}
	if (rotation < 5 * PI / 3) {
		return {-sqrt(3) * width, -width / 2} + t.translation;
	}
	return {-sqrt(3) * width, width / 2} + t.translation;
}