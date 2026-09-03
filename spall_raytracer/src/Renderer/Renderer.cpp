// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Renderer/Renderer.h>

#include <RayTracingShaders.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IPresentationFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <spall/Frame/IFrame.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>
#include <spall/Pipeline/Binding/ResourceSetCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceSetLayoutCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceWrite.h>
#include <spall/Pipeline/Pipeline/RayTracingPipelineCreateInfo.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/Buffer/BufferCreateInfo.h>
#include <spall/Resources/Texture/Texture2DCreateInfo.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>
#include <spall/SwapChain/SwapChainCreateInfo.h>
#include <src/Camera/Camera.h>
#include <src/Renderer/RendererConfig.h>

#include <span>

spall::Status Renderer::initialize(
	HWND window,
	const Scene& scene)
{
	spall::Status status = spall::createBackend(BackendType, &m_Backend);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::DeviceCreateInfo deviceInfo = {};

#ifdef _DEBUG
	deviceInfo.Debug = true;
#endif

	status = m_Backend->createDevice(deviceInfo, &m_Device);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = checkCapabilities();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::SwapChainCreateInfo swapChainInfo = {};
	swapChainInfo.Window.Value = window;
	swapChainInfo.Width = WindowWidth;
	swapChainInfo.Height = WindowHeight;
	swapChainInfo.Format = OutputFormat;

	status = m_Device->presentation().createSwapChain(swapChainInfo, &m_SwapChain);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	if (m_SwapChain->format() != OutputFormat)
	{
		return spall::ERR_UNSUPPORTED_FORMAT;
	}

	status = createOutput();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = buildScene(scene);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = createPipeline();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	m_CommandLists.resize(m_SwapChain->frameCount());

	return {};
}

spall::Status Renderer::checkCapabilities(
	void) const
{
	const spall::DeviceLimits& limits = m_Device->limits();

	if (not limits.SupportsRayTracingPipeline)
	{
		return spall::ERR_UNSUPPORTED;
	}

	if (limits.MaxRayRecursionDepth < MaxRecursionDepth)
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::FormatCapabilities capabilities = {};
	const spall::Status status = m_Device->queryFormatCapabilities(OutputFormat, &capabilities);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	constexpr spall::TextureUsageFlags required =
		spall::TextureUsageFlags::Storage | spall::TextureUsageFlags::TransferSource;

	if ((capabilities.SupportedTextureUsages & required) != required)
	{
		return spall::ERR_UNSUPPORTED_FORMAT;
	}

	return {};
}

spall::Status Renderer::createOutput(
	void)
{
	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = WindowWidth;
	textureInfo.Height = WindowHeight;
	textureInfo.Format = OutputFormat;
	textureInfo.Usage = spall::TextureUsageFlags::Storage | spall::TextureUsageFlags::TransferSource;

	const spall::Status status = m_Device->resources().createTexture2D(textureInfo, &m_Output);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::TextureViewCreateInfo viewInfo = {};
	viewInfo.Texture = m_Output.get();
	viewInfo.Aspects = spall::TextureAspectFlags::Color;

	return m_Device->resources().createTextureView(viewInfo, &m_OutputView);
}

spall::Status Renderer::buildScene(
	const Scene& scene)
{
	spall::Status status = m_SceneResources.initialize(*m_Device, scene);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::BufferCreateInfo constantsInfo = {};
	constantsInfo.Size = 256;
	constantsInfo.Usage = spall::BufferUsageFlags::Uniform;
	constantsInfo.CpuAccess = spall::MemoryAccess::Write;

	status = m_Device->resources().createBuffer(constantsInfo, &m_Constants);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const FrameConstants constants = scene.camera().constants(
		static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight));

	status = m_Device->resources().writeBuffer(*m_Constants, std::span {&constants, 1});

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::Resource<spall::ICommandList> setup;
	status = m_Device->createCommandList(&setup);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = setup->begin();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = m_SceneResources.recordBuilds(*setup);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = setup->end();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = m_Device->graphicsQueue().submit(*setup);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	return m_Device->graphicsQueue().waitIdle();
}

spall::Status Renderer::createPipeline(
	void)
{
	spall::ShaderCreateInfo shaderInfo = {};

	if constexpr (BackendType == spall::RenderBackendType::D3D12)
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::RayTracingLibrary});
	}
	else
	{
		shaderInfo.Bytecode = std::as_bytes(std::span {shaders::RayTracingLibrarySpirv});
	}

	shaderInfo.Stage = spall::ShaderStage::RayGeneration;
	spall::Status status = m_Device->pipelines().createShader(shaderInfo, &m_RayGenerationShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	shaderInfo.Stage = spall::ShaderStage::Miss;
	status = m_Device->pipelines().createShader(shaderInfo, &m_MissShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	shaderInfo.Stage = spall::ShaderStage::ClosestHit;
	status = m_Device->pipelines().createShader(shaderInfo, &m_ClosestHitShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	shaderInfo.Stage = spall::ShaderStage::Intersection;
	status = m_Device->pipelines().createShader(shaderInfo, &m_IntersectionShader);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const spall::ResourceBindingInfo bindings[] = {
		{SceneBinding, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::RayGeneration},
		{OutputBinding, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::RayGeneration},
		{ConstantsBinding, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::RayGeneration}};

	spall::ResourceSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.Bindings = bindings;

	status = m_Device->pipelines().createResourceSetLayout(layoutInfo, &m_ResourceSetLayout);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const spall::IResourceSetLayout* const layouts[] = {m_ResourceSetLayout.get()};
	const spall::PipelineShaderStageInfo missShaders[] = {{m_MissShader.get(), "missMain"}};
	const spall::RayTracingHitGroup hitGroups[] = {
		{{m_ClosestHitShader.get(), "closestHitMain"}, {}, {}},
		{{m_ClosestHitShader.get(), "proceduralClosestHitMain"}, {}, {m_IntersectionShader.get(), "intersectionMain"}}};

	spall::RayTracingPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.RayGenerationShader = {m_RayGenerationShader.get(), "rayGenMain"};
	pipelineInfo.MissShaders = missShaders;
	pipelineInfo.HitGroups = hitGroups;
	pipelineInfo.MaxPayloadSize = MaxPayloadSize;
	pipelineInfo.MaxAttributeSize = MaxAttributeSize;
	pipelineInfo.MaxRecursionDepth = MaxRecursionDepth;
	pipelineInfo.ResourceSetLayouts = layouts;

	status = m_Device->pipelines().createRayTracingPipeline(pipelineInfo, &m_Pipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	spall::ResourceWrite writes[3] = {};
	writes[0].Binding = SceneBinding;
	writes[0].Type = spall::ResourceBindingType::AccelerationStructure;
	writes[0].AccelerationStructure = &m_SceneResources.topLevel();
	writes[1].Binding = OutputBinding;
	writes[1].Type = spall::ResourceBindingType::StorageTexture;
	writes[1].TextureView = m_OutputView.get();
	writes[2].Binding = ConstantsBinding;
	writes[2].Type = spall::ResourceBindingType::UniformBuffer;
	writes[2].Buffer = m_Constants.get();

	spall::ResourceSetCreateInfo setInfo = {};
	setInfo.Layout = m_ResourceSetLayout.get();
	setInfo.Writes = writes;

	return m_Device->pipelines().createResourceSet(setInfo, &m_ResourceSet);
}

spall::Status Renderer::renderFrame(
	void)
{
	spall::Resource<spall::IFrame> frame;
	spall::Status status = m_Device->graphicsQueue().acquireFrame(*m_SwapChain, &frame);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	const std::uint32_t frameIndex = frame->index();

	if (not m_CommandLists[frameIndex])
	{
		status = m_Device->createCommandList(&m_CommandLists[frameIndex]);

		if (status != spall::SUCCESS)
		{
			return status;
		}
	}

	spall::ICommandList& commands = *m_CommandLists[frameIndex];

	status = commands.begin();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.bindRayTracingPipeline(*m_Pipeline);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.bindResourceSet(0, *m_ResourceSet);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.dispatchRays(WindowWidth, WindowHeight, 1);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.copyTexture(frame->presentTexture(), *m_Output);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = commands.end();

	if (status != spall::SUCCESS)
	{
		return status;
	}

	status = m_Device->graphicsQueue().submit(commands);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	return m_Device->graphicsQueue().present(*frame);
}

void Renderer::waitIdle(
	void)
{
	if (m_Device)
	{
		m_Device->graphicsQueue().waitIdle();
	}
}
