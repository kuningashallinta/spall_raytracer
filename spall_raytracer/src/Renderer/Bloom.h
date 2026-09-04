// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/IDevice.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Texture/ITexture2D.h>
#include <spall/Resources/TextureView/ITextureView.h>

#include <glm/vec2.hpp>

#include <vector>

struct BloomConstants
{
	glm::vec2 TexelSize;
	float Threshold;
	float Knee;
	float Radius;
};

class Bloom
{
public:
	void setThreshold(float threshold);
	void setKnee(float knee);
	void setRadius(float radius);

	spall::Status initialize(
		spall::IDevice& device,
		spall::ITextureView& source);

	spall::Status record(spall::ICommandList& commands);

	spall::ITextureView& result(void) const;

private:
	spall::Status createChain(spall::IDevice& device);
	spall::Status createPipelines(spall::IDevice& device);

	spall::Status createResourceSets(
		spall::IDevice& device,
		spall::ITextureView& source);

	spall::Status recordPass(
		spall::ICommandList& commands,
		spall::IPipeline& pipeline,
		std::uint32_t pass,
		std::uint32_t target,
		const BloomConstants& constants);

	spall::Resource<spall::ITexture2D> m_Chain;
	std::vector<spall::Resource<spall::ITextureView>> m_Views;
	spall::Resource<spall::ISampler> m_Sampler;

	spall::Resource<spall::IShader> m_ThresholdShader;
	spall::Resource<spall::IShader> m_DownsampleShader;
	spall::Resource<spall::IShader> m_UpsampleShader;

	spall::Resource<spall::IResourceSetLayout> m_ResourceSetLayout;
	std::vector<spall::Resource<spall::IResourceSet>> m_ResourceSets;

	spall::Resource<spall::IPipeline> m_ThresholdPipeline;
	spall::Resource<spall::IPipeline> m_DownsamplePipeline;
	spall::Resource<spall::IPipeline> m_UpsamplePipeline;

	float m_Threshold = 1.0f;
	float m_Knee = 0.5f;
	float m_Radius = 1.0f;
};
