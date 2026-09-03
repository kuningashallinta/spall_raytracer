// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <src/Scene/FrameConstants.h>
#include <src/Scene/Shape.h>

#include <cstdint>
#include <span>
#include <vector>

struct SceneGeometry
{
	const void* Source;
	GeometryType Type;
	std::uint32_t Offset;
	std::uint32_t Count;
};

struct SceneInstance
{
	glm::mat3x4 Transform;
	std::uint32_t InstanceId;
	std::uint32_t InstanceContribution;
	std::uint32_t GeometryIndex;
};

struct MaterialRecord
{
	glm::vec4 Albedo;
	std::uint32_t Type;
	std::uint32_t Unused[3];
};

struct InstanceRecord
{
	std::uint32_t MaterialIndex;
	std::uint32_t FirstVertex;
	std::uint32_t Unused[2];
};

class Scene
{
public:
	void build(void);

	MaterialIndex createMaterial(const Material& material);
	void add(const Shape& shape);

	std::span<const Vertex> vertices(void) const;
	std::span<const spall::AccelerationStructureAabb> aabbs(void) const;
	std::span<const SceneGeometry> geometries(void) const;
	std::span<const SceneInstance> instances(void) const;
	std::span<const MaterialRecord> materials(void) const;
	std::span<const InstanceRecord> instanceRecords(void) const;

	FrameConstants frameConstants(float aspectRatio) const;

private:
	std::uint32_t addGeometry(const Shape& shape);

	std::vector<Vertex> m_Vertices;
	std::vector<spall::AccelerationStructureAabb> m_Aabbs;
	std::vector<SceneGeometry> m_Geometries;
	std::vector<SceneInstance> m_Instances;

	std::vector<Material> m_Materials;
	std::vector<MaterialRecord> m_MaterialRecords;
	std::vector<InstanceRecord> m_InstanceRecords;

	Camera m_Camera;
	glm::vec3 m_LightDirection = {0.4f, 0.8f, -0.45f};
	glm::vec3 m_LightColor = {1.0f, 0.96f, 0.9f};
	float m_Ambient = 0.08f;
};
