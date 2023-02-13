#include "maze.h"
#include <random>
#include <map>

constexpr double PI = 3.14159265358979323846;

struct SimpleColor {
	float r, g, b;
};

struct SimpleVertex {
	float x;
	float y;
	float z;
	SimpleColor color;
};

struct Triangle {
	vertex_t t1[3];
};

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
	res.y = v.y;
	res.x = cos(angle) * v.x - sin(angle) * v.z;
	res.z = sin(angle) * v.x + cos(angle) * v.z;
	return res;
}

bool isPartOfTriangle(int x, int y, int x_off, int y_off, int size) {
	return x - x_off >= 0 && y - y_off >= 0 && x - x_off + y - y_off <= size;
}

bool isPartOfHex(int x, int y, int size) {
	return isPartOfTriangle(x, y, 0, 0, 3 * size) && !isPartOfTriangle(x, y, 0, 0, size - 1) && !isPartOfTriangle(x, y, 2 * size + 1, 0, size - 1) && !isPartOfTriangle(x, y, 0, 2 * size + 1, size - 1);
}

Vector2 coordsToHexOffset(int x, int y, float length, float width) {
	return {float((2 * x + y) * (length + width * sqrt(3)) / 2), float(y * (length + width * sqrt(3)) / 2 * sqrt(3))};
}

struct node {
	int x;
	int y;
};

struct edge {
	node n1;
	node n2;
};

std::pair<int, int> Find(std::pair<int, int> p, std::map<std::pair<int, int>, std::pair<int, int>>& leaders) {
	return (leaders[p] == p ? p : leaders[p] = Find(leaders[p], leaders));
}

void Union(std::pair<int, int> x, std::pair<int, int> y, std::map<std::pair<int, int>, std::pair<int, int>>& leaders) {
	std::pair<int, int> leader_x = Find(x, leaders), leader_y = Find(y, leaders);
	if (leader_x == leader_y) {
		return;
	}
	leaders[leader_y] = leader_x;
}

Maze getMaze(float length, float width, float height, int side_edges, int seed) {
	Maze res;
	// Cuboid
	std::vector<Triangle> cuboid;
	SimpleVertex cuboid_verticies[8];
	for (int h = 0; h <= 1; h++) {
		for (int w = 0; w <= 1; w++) {
			for (int l = 0; l <= 1; l++) {
				cuboid_verticies[l * 1 + w * 2 + h * 4] = {l == 0 ? -length / 2 : length / 2, h == 0 ? 0 : height, w == 0 ? -width / 2 : width / 2};
			}
		}
	}
	std::vector<Triangle> temp;
	// top
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[6], cuboid_verticies[7], cuboid_verticies[5]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// right
	temp = makeConvexShape({cuboid_verticies[5], cuboid_verticies[7], cuboid_verticies[3], cuboid_verticies[1]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// front
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[5], cuboid_verticies[1], cuboid_verticies[0]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// bottom
	temp = makeConvexShape({cuboid_verticies[0], cuboid_verticies[1], cuboid_verticies[3], cuboid_verticies[2]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// left
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[0], cuboid_verticies[2], cuboid_verticies[6]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// back
	temp = makeConvexShape({cuboid_verticies[2], cuboid_verticies[3], cuboid_verticies[7], cuboid_verticies[6]});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
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
	for (int i = 6; i < 12; i++) {
		hexprism_verticies[i] = hexprism_verticies[i - 6];
		hexprism_verticies[i].y = height;
	}
	// top
	temp = makeConvexShape({hexprism_verticies[0], hexprism_verticies[1], hexprism_verticies[2],hexprism_verticies[3],hexprism_verticies[4],hexprism_verticies[5]});
	hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	// bottom
	temp = makeConvexShape({hexprism_verticies[11], hexprism_verticies[10], hexprism_verticies[9],hexprism_verticies[8],hexprism_verticies[7],hexprism_verticies[6]});
	hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	for (int i = 0; i < 6; i++) {
		temp = makeConvexShape({hexprism_verticies[i], hexprism_verticies[i + 6], hexprism_verticies[((i + 1) % 6) + 6], hexprism_verticies[(i + 1) % 6]});
		hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	}
	for (int i = 0; i < hexprism.size(); i++) {
		res.hexprism[i * 3] = hexprism[i].t1[0];
		res.hexprism[i * 3 + 1] = hexprism[i].t1[1];
		res.hexprism[i * 3 + 2] = hexprism[i].t1[2];
	}
	std::vector<edge> final_edges;
	std::vector<edge> temp_edges;

	std::map<std::pair<int, int>, std::pair<int, int>> leaders;

	for (int y = 0; y <= side_edges * 3; y++) {
		for (int x = 0; x <= side_edges * 3; x++) {
			if (isPartOfHex(x, y, side_edges)) {
				res.transformations_hexprism.push_back({coordsToHexOffset(x, y, length, width)});

				leaders[{x, y}] = {x, y};

				if (isPartOfHex(x + 1, y, side_edges)) {
					if (y == 0) {
						final_edges.push_back({{x, y}, {x + 1, y}});
					}
					else {
						temp_edges.push_back({{x, y}, {x + 1, y}});
					}
				}
				if (isPartOfHex(x, y + 1, side_edges)) {
					if (x == 0) {
						final_edges.push_back({{x, y}, {x, y + 1}});
					}
					else {
						temp_edges.push_back({{x, y}, {x, y + 1}});
					}
				}
				if (isPartOfHex(x - 1, y + 1, side_edges)) {
					if (x + y == 3 * side_edges) {
						final_edges.push_back({{x, y}, {x - 1, y + 1}});
					}
					else {
						temp_edges.push_back({{x, y}, {x - 1, y + 1}});
					}
				}
			}
		}
	}

	std::mt19937 random_gen;
	random_gen.seed(seed);

	for (auto& edge : final_edges) {
		Union({edge.n1.x, edge.n1.y}, {edge.n2.x, edge.n2.y}, leaders);
	}

	std::shuffle(temp_edges.begin(), temp_edges.end(), random_gen);

	for (auto& edge : temp_edges) {
		if (Find({edge.n1.x, edge.n1.y}, leaders) != Find({edge.n2.x, edge.n2.y}, leaders)) {
			final_edges.push_back(edge);
			Union({edge.n1.x, edge.n1.y}, {edge.n2.x, edge.n2.y}, leaders);
		}
	}

	for (auto& edge : final_edges) {
		Vector2 v1 = coordsToHexOffset(edge.n1.x, edge.n1.y, length, width), v2 = coordsToHexOffset(edge.n2.x, edge.n2.y, length, width);
		res.transformations_cuboid.push_back({
			{(v1.x + v2.x) / 2, (v1.y + v2.y) / 2},
			float((edge.n1.x - edge.n2.x == 0) ? -2 * PI / 3 :
			((edge.n1.y - edge.n2.y == 0) ? 0 : 2 * PI / 3))
			});
	}

	return res;
}