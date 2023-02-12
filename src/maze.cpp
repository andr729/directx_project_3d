#include "maze.h"
#include <math.h>

# define PI          3.14159265358979323846 


SimpleColor color = {1.0f, 1.0f, 1.0f};

constexpr Triangle makeSingleTriangle(SimpleVertex p1, SimpleVertex p2, SimpleVertex p3) {
	float Ax = p2.x - p1.x;
	float Ay = p2.y - p1.y;
	float Az = p2.z - p1.z;

	float Bx = p3.x - p1.x;
	float By = p3.y - p1.y;
	float Bz = p3.z - p1.z;

	float Nx = Ay * Bz - Az * By;
	float Ny = Az * Bx - Ax * Bz;
	float Nz = Ax * By - Ay * Bx;


	return { {
		{p1.x, p1.y, p1.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f},
		{p2.x, p2.y, p2.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f},
		{p3.x, p3.y, p3.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f},
	} };
}

std::vector<Triangle> makeConvexShape(std::vector<SimpleVertex> p) {
	std::vector<Triangle> res;
	for (int i = 1; i < p.size() - 1; i++) {
		res.push_back(makeSingleTriangle(p[0], p[i], p[i + 1]));
	}
	return res;
}

SimpleVertex rotateXZ(SimpleVertex v, float angle) {
	SimpleVertex res;
	res.z = v.z;
	res.x = cos(angle) * v.x - sin(angle) * v.y;
	res.y = sin(angle) * v.x + cos(angle) * v.y;
	return res;
}

Maze getMaze(float length, float width, float height, int side_points) {
	Maze res;

	// Cuboid
	std::vector<Triangle> cuboid;
	SimpleVertex cuboid_verticies[8];
	for (int h = 0; h <= 1; h++) {
		for (int w = 0; w <= 1; w++) {
			for (int l = 0; l <= 1; l++) {
				cuboid_verticies[l * 1 + w * 2 + h * 4] = { l == 0 ? -length / 2 : length / 2, h == 0 ? 0 : height, w == 0 ? -width / 2 : width / 2 };
			}
		}
	}
	// top
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[4], cuboid_verticies[6], cuboid_verticies[7], cuboid_verticies[5]}));
	// right
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[5], cuboid_verticies[7], cuboid_verticies[3], cuboid_verticies[1]}));
	// front
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[4], cuboid_verticies[5], cuboid_verticies[1], cuboid_verticies[0]}));
	// bottom
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[0], cuboid_verticies[1], cuboid_verticies[3], cuboid_verticies[2]}));
	// left
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[4], cuboid_verticies[0], cuboid_verticies[2], cuboid_verticies[6]}));
	// back
	cuboid.emplace_back(makeConvexShape({cuboid_verticies[2], cuboid_verticies[3], cuboid_verticies[7], cuboid_verticies[6]}));
	for (int i = 0; i < cuboid.size(); i++) {
		res.cuboid[i * 3] = cuboid[i].t1[0];
		res.cuboid[i * 3 + 1] = cuboid[i].t1[1];
		res.cuboid[i * 3 + 2] = cuboid[i].t1[2];
	}

	// HexPrism
	std::vector<Triangle> hexprism;
	SimpleVertex hexprism_verticies[12];
	hexprism_verticies[0] = {0, 0, width};
	for (int i = 1; i < 6; i++) {
		hexprism_verticies[i] = rotateXZ(hexprism_verticies[0], 2 * PI * i / 6);
	}
	hexprism_verticies[6] = {0, height, width};
	for (int i = 6; i < 12; i++) {
		hexprism_verticies[i] = hexprism_verticies[i - 6];
		hexprism_verticies[i].y = height;
	}
	// top
	hexprism.emplace_back(makeConvexShape({hexprism_verticies[0], hexprism_verticies[1], hexprism_verticies[2],hexprism_verticies[3],hexprism_verticies[4],hexprism_verticies[5]}));
	// bottom
	hexprism.emplace_back(makeConvexShape({hexprism_verticies[11], hexprism_verticies[10], hexprism_verticies[9],hexprism_verticies[8],hexprism_verticies[7],hexprism_verticies[6]}));
	for (int i = 0; i < 6; i++) {
		hexprism.emplace_back(makeConvexShape({hexprism_verticies[i + 6], hexprism_verticies[i], hexprism_verticies[(i + 1) % 6], hexprism_verticies[((i + 1) % 6) + 6]}));
	}
	for (int i = 0; i < hexprism.size(); i++) {
		res.hexprism[i * 3] = hexprism[i].t1[0];
		res.hexprism[i * 3 + 1] = hexprism[i].t1[1];
		res.hexprism[i * 3 + 2] = hexprism[i].t1[2];
	}

	return res;
}