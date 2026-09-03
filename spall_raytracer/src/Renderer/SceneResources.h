// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/IDevice.h>
#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <spall/Resources/Buffer/IBuffer.h>

#include <src/Scene/Scene.h>

#include <vector>

class SceneResources
{
public:
	spall::Status initialize(
		spall::IDevice& device,
		const Scene& scene);

	spall::Status recordBuilds(spall::ICommandList& commands);

	spall::IAccelerationStructure& topLevel(void) const;
	spall::IBuffer& vertexBuffer(void) const;
	spall::IBuffer& materialBuffer(void) const;

private:
	spall::Resource<spall::IBuffer> m_Vertices;
	spall::Resource<spall::IBuffer> m_Aabbs;
	spall::Resource<spall::IBuffer> m_Instances;
	spall::Resource<spall::IBuffer> m_Materials;

	std::vector<spall::Resource<spall::IAccelerationStructure>> m_BottomLevel;
	spall::Resource<spall::IAccelerationStructure> m_TopLevel;
};
