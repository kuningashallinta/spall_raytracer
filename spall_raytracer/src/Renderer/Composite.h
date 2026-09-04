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
#include <spall/Resources/TextureView/ITextureView.h>

struct CompositeConstants
{
	float Exposure;
	float Intensity;
};

class Composite
{
public:
	void setExposure(float exposure);
	void setIntensity(float intensity);

	spall::Status initialize(
		spall::IDevice& device,
		spall::ITextureView& scene,
		spall::ITextureView& bloom,
		spall::ITextureView& output);

	spall::Status record(spall::ICommandList& commands);

private:
	spall::Resource<spall::ISampler> m_Sampler;
	spall::Resource<spall::IShader> m_Shader;

	spall::Resource<spall::IResourceSetLayout> m_ResourceSetLayout;
	spall::Resource<spall::IResourceSet> m_ResourceSet;
	spall::Resource<spall::IPipeline> m_Pipeline;

	float m_Exposure = 1.0f;
	float m_Intensity = 0.05f;
};
