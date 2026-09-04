// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Renderer/Bloom.h>

#include <BloomShaders.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <spall/Pipeline/Binding/ResourceSetCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceSetLayoutCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceWrite.h>
#include <spall/Pipeline/Pipeline/ComputePipelineCreateInfo.h>
#include <spall/Resources/Sampler/SamplerCreateInfo.h>
#include <spall/Resources/Texture/Texture2DCreateInfo.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>
#include <src/Renderer/RendererConfig.h>

#include <algorithm>
#include <span>

static std::uint32_t levelExtent(
	std::uint32_t extent,
	std::uint32_t level)
{
	return std::max(extent >> (level + 1), 1u);
}

static std::uint32_t groupCount(
	std::uint32_t extent)
{
	return (extent + GroupSize - 1) / GroupSize;
}

static glm::vec2 texelSize(
	std::uint32_t width,
	std::uint32_t height)
{
	return {1.0f / static_cast<float>(width), 1.0f / static_cast<float>(height)};
}

static spall::TextureSubresourceRange mipRange(
	std::uint32_t level)
{
	spall::TextureSubresourceRange range = {};
	range.BaseMipLevel = level;
	range.MipLevels = 1;
	range.BaseArrayLayer = 0;
	range.ArrayLayers = 1;

	return range;
}

void Bloom::setThreshold(
	float threshold)
{
	m_Threshold = threshold;
}

void Bloom::setKnee(
	float knee)
{
	m_Knee = knee;
}

void Bloom::setRadius(
	float radius)
{
	m_Radius = radius;
}

spall::Status Bloom::initialize(
	spall::IDevice& device,
	spall::ITextureView& source)
{
	spall::Status status = createChain(device);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = createPipelines(device);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	return createResourceSets(device, source);
}

spall::Status Bloom::createChain(
	spall::IDevice& device)
{
	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = levelExtent(WindowWidth, 0);
	textureInfo.Height = levelExtent(WindowHeight, 0);
	textureInfo.MipLevels = BloomLevels;
	textureInfo.Format = BloomFormat;
	textureInfo.Usage = spall::TextureUsageFlags::Storage | spall::TextureUsageFlags::Sampled;

	spall::Status status = device.resources().createTexture2D(textureInfo, &m_Chain);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	m_Views.resize(BloomLevels);

	for (std::uint32_t i = 0; i < BloomLevels; ++i)
	{
		spall::TextureViewCreateInfo viewInfo = {};
		viewInfo.Texture = m_Chain.get();
		viewInfo.Aspects = spall::TextureAspectFlags::Color;
		viewInfo.BaseMipLevel = i;
		viewInfo.MipLevels = 1;
		viewInfo.ArrayLayers = 1;

		status = device.resources().createTextureView(viewInfo, &m_Views[i]);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	spall::SamplerCreateInfo samplerInfo = {};
	samplerInfo.AddressModeU = spall::AddressMode::ClampToEdge;
	samplerInfo.AddressModeV = spall::AddressMode::ClampToEdge;
	samplerInfo.AddressModeW = spall::AddressMode::ClampToEdge;

	return device.resources().createSampler(samplerInfo, &m_Sampler);
}

spall::Status Bloom::createPipelines(
	spall::IDevice& device)
{
	const spall::ResourceBindingInfo bindings[] = {
		{BloomSourceBinding, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Compute},
		{BloomTargetBinding, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute}};

	spall::ResourceSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.Bindings = bindings;

	spall::Status status = device.pipelines().createResourceSetLayout(layoutInfo, &m_ResourceSetLayout);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::ShaderCreateInfo shaderInfo = {};
	shaderInfo.Stage = spall::ShaderStage::Compute;

	if constexpr (BackendType == spall::RenderBackendType::D3D12)
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::ThresholdMain});
	}
	else
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::ThresholdMainSpirv});
	}

	status = device.pipelines().createShader(shaderInfo, &m_ThresholdShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	if constexpr (BackendType == spall::RenderBackendType::D3D12)
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::DownsampleMain});
	}
	else
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::DownsampleMainSpirv});
	}

	status = device.pipelines().createShader(shaderInfo, &m_DownsampleShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	if constexpr (BackendType == spall::RenderBackendType::D3D12)
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::UpsampleMain});
	}
	else
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::UpsampleMainSpirv});
	}

	status = device.pipelines().createShader(shaderInfo, &m_UpsampleShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const spall::IResourceSetLayout* const layouts[] = {m_ResourceSetLayout.get()};

	spall::ComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.ResourceSetLayouts = layouts;
	pipelineInfo.PushConstants = {spall::ShaderStageFlags::Compute, sizeof(BloomConstants)};
	pipelineInfo.ComputeShader = {m_ThresholdShader.get(), "thresholdMain"};

	status = device.pipelines().createComputePipeline(pipelineInfo, &m_ThresholdPipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	pipelineInfo.ComputeShader = {m_DownsampleShader.get(), "downsampleMain"};

	status = device.pipelines().createComputePipeline(pipelineInfo, &m_DownsamplePipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	pipelineInfo.ComputeShader = {m_UpsampleShader.get(), "upsampleMain"};

	return device.pipelines().createComputePipeline(pipelineInfo, &m_UpsamplePipeline);
}

spall::Status Bloom::createResourceSets(
	spall::IDevice& device,
	spall::ITextureView& source)
{
	const std::uint32_t count = (2 * BloomLevels) - 1;

	std::vector<spall::ITextureView*> sources(count);
	std::vector<spall::ITextureView*> targets(count);

	sources[0] = &source;
	targets[0] = m_Views[0].get();

	for (std::uint32_t i = 1; i < BloomLevels; ++i)
	{
		sources[i] = m_Views[i - 1].get();
		targets[i] = m_Views[i].get();
	}

	for (std::uint32_t i = BloomLevels - 1; i > 0; --i)
	{
		const std::uint32_t pass = (BloomLevels - 1) + (BloomLevels - i);

		sources[pass] = m_Views[i].get();
		targets[pass] = m_Views[i - 1].get();
	}

	m_ResourceSets.resize(count);

	for (std::uint32_t i = 0; i < count; ++i)
	{
		spall::ResourceWrite writes[2] = {};
		writes[0].Binding = BloomSourceBinding;
		writes[0].Type = spall::ResourceBindingType::SampledTexture;
		writes[0].TextureView = sources[i];
		writes[0].Sampler = m_Sampler.get();
		writes[1].Binding = BloomTargetBinding;
		writes[1].Type = spall::ResourceBindingType::StorageTexture;
		writes[1].TextureView = targets[i];

		spall::ResourceSetCreateInfo setInfo = {};
		setInfo.Layout = m_ResourceSetLayout.get();
		setInfo.Writes = writes;

		const spall::Status status = device.pipelines().createResourceSet(setInfo, &m_ResourceSets[i]);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	return {};
}

spall::Status Bloom::recordPass(
	spall::ICommandList& commands,
	spall::IPipeline& pipeline,
	std::uint32_t pass,
	std::uint32_t target,
	const BloomConstants& constants)
{
	spall::Status status = commands.setTextureState(
		*m_Chain,
		spall::ResourceStateFlags::UnorderedAccess,
		mipRange(target));

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.commitBarriers();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.bindComputePipeline(pipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.bindResourceSet(0, *m_ResourceSets[pass]);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.setPushConstants(spall::ShaderStageFlags::Compute, 0, constants);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	return commands.dispatch(
		groupCount(levelExtent(WindowWidth, target)),
		groupCount(levelExtent(WindowHeight, target)),
		1);
}

spall::Status Bloom::record(
	spall::ICommandList& commands)
{
	BloomConstants constants = {};
	constants.TexelSize = texelSize(WindowWidth, WindowHeight);
	constants.Threshold = m_Threshold;
	constants.Knee = m_Knee;
	constants.Radius = m_Radius;

	spall::Status status = recordPass(commands, *m_ThresholdPipeline, 0, 0, constants);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	for (std::uint32_t i = 1; i < BloomLevels; ++i)
	{
		constants.TexelSize = texelSize(
			levelExtent(WindowWidth, i - 1),
			levelExtent(WindowHeight, i - 1));

		status = commands.setTextureState(
			*m_Chain,
			spall::ResourceStateFlags::ShaderResource,
			mipRange(i - 1));

		if (status != spall::SUCCESS)
		{
			return status;
		}

		status = recordPass(commands, *m_DownsamplePipeline, i, i, constants);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	for (std::uint32_t i = BloomLevels - 1; i > 0; --i)
	{
		constants.TexelSize = texelSize(levelExtent(WindowWidth, i), levelExtent(WindowHeight, i));

		status = commands.setTextureState(
			*m_Chain,
			spall::ResourceStateFlags::ShaderResource,
			mipRange(i));

		if (status != spall::SUCCESS)
		{
			return status;
		}

		status = recordPass(
			commands,
			*m_UpsamplePipeline,
			(BloomLevels - 1) + (BloomLevels - i),
			i - 1,
			constants);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	return commands.setTextureState(
		*m_Chain,
		spall::ResourceStateFlags::ShaderResource,
		mipRange(0));
}

spall::ITextureView& Bloom::result(
	void) const
{
	return *m_Views[0];
}
