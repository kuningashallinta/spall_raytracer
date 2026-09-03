// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <src/Scene/Shape.h>

class Plane : public Shape
{
public:
	GeometryType geometryType(void) const override;
	std::span<const Vertex> vertices(void) const override;
	std::span<const spall::AccelerationStructureAabb> aabbs(void) const override;
};
