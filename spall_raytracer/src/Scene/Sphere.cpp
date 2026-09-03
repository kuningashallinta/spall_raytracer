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

spall::AccelerationStructureAabb Sphere::bounds(
	void) const
{
	return {-0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};
}
