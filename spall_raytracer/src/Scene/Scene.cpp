// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Scene.h>

#include <src/Scene/Plane.h>
#include <src/Scene/Sphere.h>

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <random>

void Scene::build(
	void)
{
	m_Camera.setPosition({0.0f, 1.6f, -6.0f});
	m_Camera.setTarget({0.0f, 0.1f, 0.0f});
	m_Camera.setFieldOfView(45.0f);

	const MaterialIndex slate = createMaterial({.Albedo = {0.58f, 0.58f, 0.60f}, .Roughness = 0.32f});
	const MaterialIndex chrome = createMaterial({.Albedo = {0.95f, 0.95f, 0.97f}, .Roughness = 0.04f, .Metallic = 1.0f});
	const MaterialIndex brass = createMaterial({.Albedo = {0.94f, 0.76f, 0.38f}, .Roughness = 0.22f, .Metallic = 1.0f});

	const MaterialIndex glass = createMaterial({.Albedo = {0.92f, 0.97f, 0.95f},
		.Roughness = 0.0f,
		.Transmission = 1.0f,
		.Ior = 1.52f});

	const MaterialIndex lamp = createMaterial({.Emission = {14.0f, 8.6f, 4.2f}});

	const glm::vec3 palette[] = {
		{0.85f, 0.34f, 0.14f},
		{0.22f, 0.48f, 0.36f},
		{0.28f, 0.36f, 0.68f},
		{0.74f, 0.70f, 0.32f}};

	Plane ground;
	ground.setPosition({0.0f, -0.5f, 0.0f});
	ground.setScale({40.0f, 1.0f, 40.0f});
	ground.setMaterial(slate);
	add(ground);

	Sphere mirror;
	mirror.setPosition({0.0f, 0.25f, 0.0f});
	mirror.setScale(1.5f);
	mirror.setMaterial(chrome);
	add(mirror);

	Sphere gold;
	gold.setPosition({-2.1f, 0.05f, -0.6f});
	gold.setScale(1.1f);
	gold.setMaterial(brass);
	add(gold);

	Sphere bubble;
	bubble.setPosition({2.2f, 0.1f, -1.0f});
	bubble.setScale(1.2f);
	bubble.setMaterial(glass);
	add(bubble);

	Sphere lantern;
	lantern.setPosition({-1.3f, 1.5f, -2.4f});
	lantern.setScale(0.45f);
	lantern.setMaterial(lamp);
	add(lantern);

	std::mt19937 noise(0x2545f491u);
	std::uniform_real_distribution<float> unit(0.0f, 1.0f);

	for (std::size_t i = 0; i < 11; ++i)
	{
		for (std::size_t j = 0; j < 10; ++j)
		{
			const float diameter = 0.16f + (unit(noise) * 0.38f);
			const float x = ((static_cast<float>(i) - 5.0f) * 1.15f) + ((unit(noise) - 0.5f) * 0.7f);
			const float z = ((static_cast<float>(j) - 3.0f) * 1.15f) + ((unit(noise) - 0.5f) * 0.7f);
			const float pick = unit(noise);

			if ((std::sqrt((x * x) + (z * z)) < 1.5f) or
				(std::sqrt(((x + 2.1f) * (x + 2.1f)) + ((z + 0.6f) * (z + 0.6f))) < 1.2f) or
				(std::sqrt(((x - 2.2f) * (x - 2.2f)) + ((z + 1.0f) * (z + 1.0f))) < 1.3f))
			{
				continue;
			}

			MaterialIndex material = glass;

			if (pick > 0.13f)
			{
				material = createMaterial({.Albedo = palette[static_cast<std::size_t>(unit(noise) * 4.0f) % 4],
					.Roughness = 0.04f + (unit(noise) * 0.8f),
					.Metallic = (pick < 0.42f) ? 1.0f : 0.0f});
			}
			else if (pick > 0.09f)
			{
				material = lamp;
			}

			Sphere ball;
			ball.setPosition({x, -0.5f + (diameter * 0.5f), z});
			ball.setScale(diameter);
			ball.setMaterial(material);
			add(ball);
		}
	}
}

MaterialIndex Scene::createMaterial(
	const Material& material)
{
	const auto existing = std::ranges::find(m_Materials, material);

	if (existing != m_Materials.end())
	{
		return static_cast<MaterialIndex>(existing - m_Materials.begin());
	}

	MaterialRecord record = {};
	record.Albedo = material.Albedo;
	record.Roughness = material.Roughness;
	record.Emission = material.Emission;
	record.Metallic = material.Metallic;
	record.Transmission = material.Transmission;
	record.Ior = material.Ior;

	m_Materials.push_back(material);
	m_MaterialRecords.push_back(record);

	return static_cast<MaterialIndex>(m_Materials.size() - 1);
}

std::uint32_t Scene::addGeometry(
	const Shape& shape)
{
	const GeometryType type = shape.geometryType();
	const std::span<const Vertex> vertices = shape.vertices();
	const std::span<const spall::AccelerationStructureAabb> aabbs = shape.aabbs();

	const void* const source = (type == GeometryType::Triangles)
		? static_cast<const void*>(vertices.data())
		: static_cast<const void*>(aabbs.data());

	for (std::size_t i = 0; i < m_Geometries.size(); ++i)
	{
		if ((m_Geometries[i].Source == source) and (m_Geometries[i].Type == type))
		{
			return static_cast<std::uint32_t>(i);
		}
	}

	SceneGeometry geometry = {};
	geometry.Source = source;
	geometry.Type = type;

	switch (type)
	{
		case GeometryType::Triangles:
		{
			geometry.Offset = static_cast<std::uint32_t>(m_Vertices.size() * sizeof(Vertex));
			geometry.Count = static_cast<std::uint32_t>(vertices.size());

			m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
			break;
		}

		case GeometryType::Aabbs:
		{
			geometry.Offset = static_cast<std::uint32_t>(m_Aabbs.size() * sizeof(spall::AccelerationStructureAabb));
			geometry.Count = static_cast<std::uint32_t>(aabbs.size());

			m_Aabbs.insert(m_Aabbs.end(), aabbs.begin(), aabbs.end());
			break;
		}
	}

	m_Geometries.push_back(geometry);

	return static_cast<std::uint32_t>(m_Geometries.size() - 1);
}

void Scene::add(
	const Shape& shape)
{
	const std::uint32_t geometryIndex = addGeometry(shape);
	const SceneGeometry& geometry = m_Geometries[geometryIndex];

	SceneInstance instance = {};
	instance.Transform = shape.transform();
	instance.InstanceId = static_cast<std::uint32_t>(m_Instances.size());
	instance.InstanceContribution = (geometry.Type == GeometryType::Triangles) ? 0u : 1u;
	instance.GeometryIndex = geometryIndex;

	InstanceRecord record = {};
	record.MaterialIndex = shape.material();

	if (geometry.Type == GeometryType::Triangles)
	{
		record.FirstVertex = geometry.Offset / sizeof(Vertex);
	}

	m_Instances.push_back(instance);
	m_InstanceRecords.push_back(record);
}

std::span<const Vertex> Scene::vertices(
	void) const
{
	return m_Vertices;
}

std::span<const spall::AccelerationStructureAabb> Scene::aabbs(
	void) const
{
	return m_Aabbs;
}

std::span<const SceneGeometry> Scene::geometries(
	void) const
{
	return m_Geometries;
}

std::span<const SceneInstance> Scene::instances(
	void) const
{
	return m_Instances;
}

std::span<const MaterialRecord> Scene::materials(
	void) const
{
	return m_MaterialRecords;
}

std::span<const InstanceRecord> Scene::instanceRecords(
	void) const
{
	return m_InstanceRecords;
}

FrameConstants Scene::frameConstants(
	float aspectRatio) const
{
	FrameConstants constants = {};
	constants.Camera = m_Camera.view(aspectRatio);
	constants.LightDirection = glm::vec4(glm::normalize(m_LightDirection), 0.0f);
	constants.LightColor = glm::vec4(m_LightColor, m_SkyIntensity);

	return constants;
}
