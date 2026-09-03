// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Resources/AccelerationStructure/AccelerationStructureAabb.h>

#include <src/Scene/Vertex.h>

#include <glm/vec3.hpp>

#include <cstdint>
#include <span>

enum class GeometryType : std::uint32_t
{
	Triangles,
	Aabbs
};

class Shape
{
public:
	virtual ~Shape(void) = default;

	void setPosition(const glm::vec3& position);
	void setRotation(const glm::vec3& degrees);
	void setScale(const glm::vec3& scale);
	void setScale(float scale);

	void writeTransform(float (&transform)[12]) const;

	virtual GeometryType geometryType(void) const = 0;
	virtual std::span<const Vertex> vertices(void) const = 0;
	virtual spall::AccelerationStructureAabb bounds(void) const = 0;

private:
	glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_Rotation = {0.0f, 0.0f, 0.0f};
	glm::vec3 m_Scale = {1.0f, 1.0f, 1.0f};
};
