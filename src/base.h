#pragma once

#include <cmath>

struct vertex_t {
	// @TODO: float
	float position[3];
	float normal_vector[3];
	float color[4];
};

struct Vector2 {
	float x, y;

	[[no_discard]]
	constexpr Vector2 operator+(const Vector2& oth) const {
		return {x + oth.x, y + oth.y};
	}

	[[no_discard]]
	constexpr Vector2 operator-(const Vector2& oth) const {
		return {x - oth.x, y - oth.y};
	}
	constexpr void operator+=(const Vector2& oth) {
		x += oth.x;
		y += oth.y;
	}
	constexpr void operator-=(const Vector2& oth) {
		x -= oth.x;
		y -= oth.y;
	}

	[[no_discard]]
	constexpr Vector2 operator*(const float v) const {
		return {x * v, y * v};
	}
	[[no_discard]]
	constexpr Vector2 operator/(const float v) const {
		return {x / v, y / v};
	}
	constexpr void operator*=(const float v) {
		x *= v;
		y *= v;
	}

	[[no_discard]]
	Vector2 operator-() const {
		return {-x, -y};
	}

	[[no_discard]]
	constexpr float abs2() const {
		return x * x + y * y;
	}

	[[no_discard]]
	float abs() const {
		return std::sqrt(abs2());
	}

	[[no_discard]]
	Vector2 normUnit() const {
		return *this / abs();
	}

	[[no_discard]]
	Vector2 rot90() const {
		return {-y, x};
	}

	[[no_discard]]
	Vector2 rot270() const {
		return {y, -x};
	}
};


// @TODO: float
constexpr size_t VERTEX_SIZE = sizeof(vertex_t) / sizeof(float);
