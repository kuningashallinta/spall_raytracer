// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Cube.h>

#include <vector>

static std::vector<Vertex> unitCube(
	void)
{
	const glm::vec3 corners[] = {
		{-0.5f, -0.5f, -0.5f},
		{0.5f, -0.5f, -0.5f},
		{0.5f, 0.5f, -0.5f},
		{-0.5f, 0.5f, -0.5f},
		{-0.5f, -0.5f, 0.5f},
		{0.5f, -0.5f, 0.5f},
		{0.5f, 0.5f, 0.5f},
		{-0.5f, 0.5f, 0.5f}};

	const std::uint32_t faces[] = {
		0, 2, 1, 0, 3, 2,
		4, 5, 6, 4, 6, 7,
		0, 4, 7, 0, 7, 3,
		1, 2, 6, 1, 6, 5,
		0, 1, 5, 0, 5, 4,
		3, 7, 6, 3, 6, 2};

	std::vector<Vertex> vertices;
	vertices.reserve(std::size(faces));

	for (const std::uint32_t corner : faces)
	{
		vertices.push_back({corners[corner], 0.0f});
	}

	return vertices;
}

GeometryType Cube::geometryType(
	void) const
{
	return GeometryType::Triangles;
}

std::span<const Vertex> Cube::vertices(
	void) const
{
	static const std::vector<Vertex> vertices = unitCube();

	return vertices;
}

std::span<const spall::AccelerationStructureAabb> Cube::aabbs(
	void) const
{
	return {};
}
