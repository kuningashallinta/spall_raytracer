// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <src/Camera/Camera.h>
#include <src/Scene/Shape.h>

#include <cstdint>
#include <span>
#include <vector>

struct SceneInstance
{
	float Transform[12];
	std::uint32_t InstanceId;
	std::uint32_t InstanceContribution;
	GeometryType Type;
	std::uint32_t GeometryOffset;
	std::uint32_t GeometryCount;
};

class Scene
{
public:
	void build(void);
	void add(const Shape& shape);

	std::span<const Vertex> vertices(void) const;
	std::span<const spall::AccelerationStructureAabb> aabbs(void) const;
	std::span<const SceneInstance> instances(void) const;
	const Camera& camera(void) const;

private:
	std::vector<Vertex> m_Vertices;
	std::vector<spall::AccelerationStructureAabb> m_Aabbs;
	std::vector<SceneInstance> m_Instances;
	Camera m_Camera;
};
