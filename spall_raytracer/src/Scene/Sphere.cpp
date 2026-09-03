// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Sphere.h>

GeometryType Sphere::geometryType(
	void) const
{
	return GeometryType::Aabbs;
}

std::span<const Vertex> Sphere::vertices(
	void) const
{
	return {};
}

std::span<const spall::AccelerationStructureAabb> Sphere::aabbs(
	void) const
{
	static const spall::AccelerationStructureAabb box = {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

	return {&box, 1};
}
