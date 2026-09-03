// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Plane.h>

GeometryType Plane::geometryType(
	void) const
{
	return GeometryType::Triangles;
}

std::span<const Vertex> Plane::vertices(
	void) const
{
	static const Vertex vertices[] = {
		{{-0.5f, 0.0f, -0.5f}, 0.0f},
		{{0.5f, 0.0f, -0.5f}, 0.0f},
		{{0.5f, 0.0f, 0.5f}, 0.0f},
		{{-0.5f, 0.0f, -0.5f}, 0.0f},
		{{0.5f, 0.0f, 0.5f}, 0.0f},
		{{-0.5f, 0.0f, 0.5f}, 0.0f}};

	return vertices;
}

std::span<const spall::AccelerationStructureAabb> Plane::aabbs(
	void) const
{
	return {};
}
