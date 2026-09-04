// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Renderer/Composite.h>

#include <CompositeShaders.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <spall/Pipeline/Binding/ResourceSetCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceSetLayoutCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceWrite.h>
#include <spall/Pipeline/Pipeline/ComputePipelineCreateInfo.h>
#include <spall/Resources/Sampler/SamplerCreateInfo.h>
#include <src/Renderer/RendererConfig.h>

#include <span>

void Composite::setExposure(
	float exposure)
{
	m_Exposure = exposure;
}

void Composite::setIntensity(
	float intensity)
{
	m_Intensity = intensity;
}

spall::Status Composite::initialize(
	spall::IDevice& device,
	spall::ITextureView& scene,
	spall::ITextureView& bloom,
	spall::ITextureView& output)
{
	spall::SamplerCreateInfo samplerInfo = {};
	samplerInfo.AddressModeU = spall::AddressMode::ClampToEdge;
	samplerInfo.AddressModeV = spall::AddressMode::ClampToEdge;
	samplerInfo.AddressModeW = spall::AddressMode::ClampToEdge;

	spall::Status status = device.resources().createSampler(samplerInfo, &m_Sampler);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const spall::ResourceBindingInfo bindings[] = {
		{CompositeSceneBinding, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute},
		{CompositeBloomBinding, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Compute},
		{CompositeOutputBinding, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute}};

	spall::ResourceSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.Bindings = bindings;

	status = device.pipelines().createResourceSetLayout(layoutInfo, &m_ResourceSetLayout);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::ShaderCreateInfo shaderInfo = {};
	shaderInfo.Stage = spall::ShaderStage::Compute;

	if constexpr (BackendType == spall::RenderBackendType::D3D12)
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::CompositeMain});
	}
	else
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::CompositeMainSpirv});
	}

	status = device.pipelines().createShader(shaderInfo, &m_Shader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const spall::IResourceSetLayout* const layouts[] = {m_ResourceSetLayout.get()};

	spall::ComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.ResourceSetLayouts = layouts;
	pipelineInfo.PushConstants = {spall::ShaderStageFlags::Compute, sizeof(CompositeConstants)};
	pipelineInfo.ComputeShader = {m_Shader.get(), "compositeMain"};

	status = device.pipelines().createComputePipeline(pipelineInfo, &m_Pipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::ResourceWrite writes[3] = {};
	writes[0].Binding = CompositeSceneBinding;
	writes[0].Type = spall::ResourceBindingType::StorageTexture;
	writes[0].TextureView = &scene;
	writes[1].Binding = CompositeBloomBinding;
	writes[1].Type = spall::ResourceBindingType::SampledTexture;
	writes[1].TextureView = &bloom;
	writes[1].Sampler = m_Sampler.get();
	writes[2].Binding = CompositeOutputBinding;
	writes[2].Type = spall::ResourceBindingType::StorageTexture;
	writes[2].TextureView = &output;

	spall::ResourceSetCreateInfo setInfo = {};
	setInfo.Layout = m_ResourceSetLayout.get();
	setInfo.Writes = writes;

	return device.pipelines().createResourceSet(setInfo, &m_ResourceSet);
}

spall::Status Composite::record(
	spall::ICommandList& commands)
{
	spall::Status status = commands.bindComputePipeline(*m_Pipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.bindResourceSet(0, *m_ResourceSet);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	CompositeConstants constants = {};
	constants.Exposure = m_Exposure;
	constants.Intensity = m_Intensity;

	status = commands.setPushConstants(spall::ShaderStageFlags::Compute, 0, constants);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	return commands.dispatch(
		(WindowWidth + GroupSize - 1) / GroupSize,
		(WindowHeight + GroupSize - 1) / GroupSize,
		1);
}
