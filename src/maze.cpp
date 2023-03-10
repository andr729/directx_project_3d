#include "maze.hpp"

#include <random>
#include <map>

struct SimpleColor {
	float r, g, b;
};

struct Vector3 {
	float x;
	float y;
	float z;
};

struct Triangle {
	vertex_t t1[3];
};

SimpleColor color = {1.0f, 1.0f, 1.0f};

constexpr float textureCoords(Vector3 v, Vector3 texture_offset, Vector3 base) {
	return (v.x - texture_offset.x) * base.x + (v.y - texture_offset.y) * base.y + (v.z - texture_offset.z) * base.z;
}

constexpr Triangle makeSingleTriangle(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 texture_offset, Vector3 basex, Vector3 basey) {
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
		{p1.x, p1.y, p1.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f,   textureCoords(p1, texture_offset, basex), textureCoords(p1, texture_offset, basey)},
		{p2.x, p2.y, p2.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f,   textureCoords(p2, texture_offset, basex), textureCoords(p2, texture_offset, basey)},
		{p3.x, p3.y, p3.z,     Nx, Ny, Nz,    color.r, color.g, color.b, 1.0f,   textureCoords(p3, texture_offset, basex), textureCoords(p3, texture_offset, basey)},
	} };
}

std::vector<Triangle> makeConvexShape(std::vector<Vector3> p, Vector3 texture_offset, Vector3 basex, Vector3 basey) {
	std::vector<Triangle> res;
	for (int i = 1; i < p.size() - 1; i++) {
		res.push_back(makeSingleTriangle(p[0], p[i], p[i + 1], texture_offset, basex, basey));
	}
	return res;
}

Vector3 rotateXZ(Vector3 v, float angle) {
	Vector3 res;
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
	float texturevec = 1 / std::max(std::max(height, length), 2 * width);
	Maze res;
	// Cuboid
	std::vector<Triangle> cuboid;
	Vector3 cuboid_verticies[8];
	for (int h = 0; h <= 1; h++) {
		for (int w = 0; w <= 1; w++) {
			for (int l = 0; l <= 1; l++) {
				cuboid_verticies[l * 1 + w * 2 + h * 4] = {l == 0 ? -length / 2 : length / 2, h == 0 ? 0 : height, w == 0 ? -width / 2 : width / 2};
			}
		}
	}
	std::vector<Triangle> temp;
	// top
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[6], cuboid_verticies[7], cuboid_verticies[5]}, cuboid_verticies[4], {texturevec, 0, 0}, {0, 0, texturevec});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// right
	temp = makeConvexShape({cuboid_verticies[5], cuboid_verticies[7], cuboid_verticies[3], cuboid_verticies[1]}, cuboid_verticies[1], {0, 0, texturevec}, {0, texturevec, 0});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// front
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[5], cuboid_verticies[1], cuboid_verticies[0]}, cuboid_verticies[0], {texturevec, 0, 0}, {0, texturevec, 0});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// bottom
	temp = makeConvexShape({cuboid_verticies[0], cuboid_verticies[1], cuboid_verticies[3], cuboid_verticies[2]}, cuboid_verticies[3], {-texturevec, 0, 0}, {0, 0, -texturevec});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// left
	temp = makeConvexShape({cuboid_verticies[4], cuboid_verticies[0], cuboid_verticies[2], cuboid_verticies[6]}, cuboid_verticies[0], {0, 0, texturevec}, {0, texturevec, 0});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	// back
	temp = makeConvexShape({cuboid_verticies[2], cuboid_verticies[3], cuboid_verticies[7], cuboid_verticies[6]}, cuboid_verticies[2], {texturevec, 0, 0}, {0, texturevec});
	cuboid.insert(cuboid.end(), temp.begin(), temp.end());
	for (int i = 0; i < cuboid.size(); i++) {
		res.cuboid[i * 3] = cuboid[i].t1[0];
		res.cuboid[i * 3 + 1] = cuboid[i].t1[1];
		res.cuboid[i * 3 + 2] = cuboid[i].t1[2];
	}

	// HexPrism
	std::vector<Triangle> hexprism;
	Vector3 hexprism_verticies[12];
	hexprism_verticies[0] = {0, 0, width};
	for (int i = 1; i < 6; i++) {
		hexprism_verticies[i] = rotateXZ(hexprism_verticies[0], 2 * PI * i / 6);
	}
	for (int i = 6; i < 12; i++) {
		hexprism_verticies[i] = hexprism_verticies[i - 6];
		hexprism_verticies[i].y = height;
	}
	// top
	temp = makeConvexShape({hexprism_verticies[0], hexprism_verticies[1], hexprism_verticies[2],hexprism_verticies[3],hexprism_verticies[4],hexprism_verticies[5]}, {hexprism_verticies[0].x - width, hexprism_verticies[0].y, hexprism_verticies[0].z - 2 * width}, {texturevec, 0, 0}, {0, 0, texturevec});
	hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	// bottom
	temp = makeConvexShape({hexprism_verticies[11], hexprism_verticies[10], hexprism_verticies[9],hexprism_verticies[8],hexprism_verticies[7],hexprism_verticies[6]}, {hexprism_verticies[6].x - width, hexprism_verticies[6].y, hexprism_verticies[6].z - 2 * width}, {texturevec, 0, 0}, {0, 0, texturevec});
	hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	float z_impact[6] = {0.5, 1, 0.5, -0.5, -1, -0.5};
	float x_impact[6] = {sqrt(3) / 2, 0, -sqrt(3) / 2, -sqrt(3) / 2, 0, sqrt(3) / 2};
	for (int i = 0; i < 6; i++) {
		temp = makeConvexShape({hexprism_verticies[i], hexprism_verticies[i + 6], hexprism_verticies[((i + 1) % 6) + 6], hexprism_verticies[(i + 1) % 6]}, hexprism_verticies[(i + 1) % 6], {texturevec * x_impact[i], 0, texturevec * z_impact[i]}, {0, texturevec, 0});
		hexprism.insert(hexprism.end(), temp.begin(), temp.end());
	}
	for (int i = 0; i < hexprism.size(); i++) {
		res.hexprism[i * 3] = hexprism[i].t1[0];
		res.hexprism[i * 3 + 1] = hexprism[i].t1[1];
		res.hexprism[i * 3 + 2] = hexprism[i].t1[2];
	}

	std::vector<Triangle> floor;
	floor = makeConvexShape({{0, 0, 0}, {0, 0, length}, {length, 0, length}, {length, 0, 0}}, {0, 0, 0}, {texturevec, 0, 0}, {0, 0, texturevec});
	for (int i = 0; i < floor.size(); i++) {
		res.floor[i * 3] = floor[i].t1[0];
		res.floor[i * 3 + 1] = floor[i].t1[1];
		res.floor[i * 3 + 2] = floor[i].t1[2];
	}
	for (int z = -side_edges; z < side_edges * 4; z++) {
		for (int x = -side_edges; x < side_edges * 4; x++) {
			res.transformations_floor.push_back({{x * length, z * length}});
		}
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
					if (y == 0 || y == 2 * side_edges) {
						final_edges.push_back({{x, y}, {x + 1, y}});
					}
					else {
						temp_edges.push_back({{x, y}, {x + 1, y}});
					}
				}
				if (isPartOfHex(x, y + 1, side_edges)) {
					if (x == 0 || x == 2 * side_edges) {
						final_edges.push_back({{x, y}, {x, y + 1}});
					}
					else {
						temp_edges.push_back({{x, y}, {x, y + 1}});
					}
				}
				if (isPartOfHex(x - 1, y + 1, side_edges)) {
					if (x + y == 3 * side_edges || x + y == side_edges) {
						final_edges.push_back({{x, y}, {x - 1, y + 1}});
					}
					else {
						temp_edges.push_back({{x, y}, {x - 1, y + 1}});
					}
				}
			}
		}
	}

	res.player_coordinates = (coordsToHexOffset(side_edges, side_edges - 1, length, width) + coordsToHexOffset(side_edges, side_edges, length, width) + coordsToHexOffset(side_edges + 1, side_edges - 1, length, width)) / 3;

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
			float((edge.n1.x - edge.n2.x == 0) ? 2 * PI / 3 :
			((edge.n1.y - edge.n2.y == 0) ? 0 : -2 * PI / 3))
			});
	}

	return res;
}