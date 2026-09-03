// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Scene.h>

#include <src/Scene/Cube.h>
#include <src/Scene/Plane.h>
#include <src/Scene/Sphere.h>

void Scene::build(
	void)
{
	m_Camera.setPosition({0.0f, 1.5f, -4.0f});
	m_Camera.setTarget({0.0f, 0.0f, 0.0f});
	m_Camera.setFieldOfView(45.0f);

	Plane ground;
	ground.setPosition({0.0f, -0.5f, 0.0f});
	ground.setScale({12.0f, 1.0f, 12.0f});

	Cube cube;
	cube.setRotation({0.0f, 22.0f, 0.0f});

	Sphere sphere;
	sphere.setPosition({1.4f, 0.25f, 0.2f});
	sphere.setScale(1.5f);

	add(ground);
	add(cube);
	add(sphere);
}

void Scene::add(
	const Shape& shape)
{
	SceneInstance instance = {};
	shape.writeTransform(instance.Transform);

	instance.InstanceId = static_cast<std::uint32_t>(m_Instances.size());
	instance.Type = shape.geometryType();

	switch (instance.Type)
	{
		case GeometryType::Triangles:
		{
			const std::span<const Vertex> vertices = shape.vertices();

			instance.InstanceContribution = 0;
			instance.GeometryOffset = static_cast<std::uint32_t>(m_Vertices.size() * sizeof(Vertex));
			instance.GeometryCount = static_cast<std::uint32_t>(vertices.size());

			m_Vertices.insert(m_Vertices.end(), vertices.begin(), vertices.end());
			break;
		}

		case GeometryType::Aabbs:
		{
			instance.InstanceContribution = 1;
			instance.GeometryOffset = static_cast<std::uint32_t>(m_Aabbs.size() * sizeof(spall::AccelerationStructureAabb));
			instance.GeometryCount = 1;

			m_Aabbs.push_back(shape.bounds());
			break;
		}
	}

	m_Instances.push_back(instance);
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

std::span<const SceneInstance> Scene::instances(
	void) const
{
	return m_Instances;
}

const Camera& Scene::camera(
	void) const
{
	return m_Camera;
}
