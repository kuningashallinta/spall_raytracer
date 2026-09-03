// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Renderer/SceneResources.h>

#include <spall/Device/IResourceFactory.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureCreateInfo.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureGeometry.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.h>
#include <spall/Resources/Buffer/BufferCreateInfo.h>

#include <span>

spall::Status SceneResources::initialize(
	spall::IDevice& device,
	const Scene& scene)
{
	const std::span<const Vertex> vertices = scene.vertices();
	const std::span<const SceneInstance> instances = scene.instances();

	spall::BufferCreateInfo vertexInfo = {};
	vertexInfo.Size = static_cast<std::uint32_t>(vertices.size_bytes());
	vertexInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput | spall::BufferUsageFlags::Storage;

	spall::Status status = device.resources().createBufferWithData(
		vertexInfo,
		std::as_bytes(vertices),
		&m_Vertices);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const std::span<const spall::AccelerationStructureAabb> aabbs = scene.aabbs();

	if (not aabbs.empty())
	{
		spall::BufferCreateInfo aabbInfo = {};
		aabbInfo.Size = static_cast<std::uint32_t>(aabbs.size_bytes());
		aabbInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;

		status = device.resources().createBufferWithData(aabbInfo, std::as_bytes(aabbs), &m_Aabbs);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	m_BottomLevel.resize(instances.size());

	for (std::size_t i = 0; i < instances.size(); ++i)
	{
		spall::AccelerationStructureGeometry geometry = {};

		switch (instances[i].Type)
		{
			case GeometryType::Triangles:
			{
				geometry.Type = spall::AccelerationStructureGeometryType::Triangles;
				geometry.VertexBuffer = m_Vertices.get();
				geometry.VertexFormat = spall::Format::RGB32Float;
				geometry.VertexOffset = instances[i].GeometryOffset;
				geometry.VertexStride = sizeof(Vertex);
				geometry.VertexCount = instances[i].GeometryCount;
				break;
			}

			case GeometryType::Aabbs:
			{
				geometry.Type = spall::AccelerationStructureGeometryType::Aabbs;
				geometry.AabbBuffer = m_Aabbs.get();
				geometry.AabbOffset = instances[i].GeometryOffset;
				geometry.AabbCount = instances[i].GeometryCount;
				break;
			}
		}

		spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
		bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
		bottomLevelInfo.Geometries = std::span {&geometry, 1};

		status = device.resources().createAccelerationStructure(bottomLevelInfo, &m_BottomLevel[i]);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	std::vector<spall::AccelerationStructureInstance> records;
	records.reserve(instances.size());

	for (std::size_t i = 0; i < instances.size(); ++i)
	{
		records.push_back(spall::makeAccelerationStructureInstance(
			*m_BottomLevel[i],
			instances[i].Transform,
			instances[i].InstanceId,
			0xFF,
			spall::AccelerationStructureInstanceFlags::None,
			instances[i].InstanceContribution));
	}

	spall::BufferCreateInfo instanceInfo = {};
	instanceInfo.Size = static_cast<std::uint32_t>(records.size() * sizeof(spall::AccelerationStructureInstance));
	instanceInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;

	status = device.resources().createBufferWithData(
		instanceInfo,
		std::as_bytes(std::span {records}),
		&m_Instances);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const std::span<const MaterialRecord> materials = scene.materials();

	spall::BufferCreateInfo materialInfo = {};
	materialInfo.Size = static_cast<std::uint32_t>(materials.size_bytes());
	materialInfo.Usage = spall::BufferUsageFlags::Storage;

	status = device.resources().createBufferWithData(materialInfo, std::as_bytes(materials), &m_Materials);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const std::span<const InstanceRecord> instanceRecords = scene.instanceRecords();

	spall::BufferCreateInfo instanceRecordInfo = {};
	instanceRecordInfo.Size = static_cast<std::uint32_t>(instanceRecords.size_bytes());
	instanceRecordInfo.Usage = spall::BufferUsageFlags::Storage;

	status = device.resources().createBufferWithData(
		instanceRecordInfo,
		std::as_bytes(instanceRecords),
		&m_InstanceRecords);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = m_Instances.get();
	topLevelInfo.InstanceCount = static_cast<std::uint32_t>(records.size());

	return device.resources().createAccelerationStructure(topLevelInfo, &m_TopLevel);
}

spall::Status SceneResources::recordBuilds(
	spall::ICommandList& commands)
{
	for (const spall::Resource<spall::IAccelerationStructure>& bottomLevel : m_BottomLevel)
	{
		const spall::Status status = commands.buildAccelerationStructure(*bottomLevel);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	return commands.buildAccelerationStructure(*m_TopLevel);
}

spall::IAccelerationStructure& SceneResources::topLevel(
	void) const
{
	return *m_TopLevel;
}

spall::IBuffer& SceneResources::vertexBuffer(
	void) const
{
	return *m_Vertices;
}

spall::IBuffer& SceneResources::materialBuffer(
	void) const
{
	return *m_Materials;
}

spall::IBuffer& SceneResources::instanceRecordBuffer(
	void) const
{
	return *m_InstanceRecords;
}
