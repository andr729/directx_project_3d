#pragma once

#include <cmath>

// @TODO: dx12 float must be used, to not break everything on diffrent arch
typedef float FLOAT;

struct vertex_t {
	FLOAT position[3];
	FLOAT normal_vector[3];
	FLOAT color[4];
	FLOAT tex_coord[2];
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

	Vector2 rot(float angle) const {
		float c = std::cos(angle);
		float s = std::sin(angle);
		float rx = c*x - s*y; 
		float ry = s*x + c*y;
		return {rx, ry};
	}

	float deg() const {
		auto rx = y;
		auto ry = x;
		return std::atan2(ry, rx);
	}
};


// @TODO: float
constexpr size_t VERTEX_SIZE = sizeof(vertex_t) / sizeof(float);
