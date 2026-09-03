// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Backend/BackendFactory.h>
#include <spall/CommandList/ICommandList.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/IDevice.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Texture/ITexture2D.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/SwapChain/ISwapChain.h>

#include <src/Renderer/SceneResources.h>

#include <windows.h>

#include <memory>
#include <vector>

class Renderer
{
public:
	spall::Status initialize(
		HWND window,
		const Scene& scene);
	spall::Status renderFrame(void);

	void waitIdle(void);

private:
	spall::Status checkCapabilities(void) const;
	spall::Status createPipeline(void);
	spall::Status buildScene(const Scene& scene);
	spall::Status createOutput(void);

	std::unique_ptr<spall::IBackend> m_Backend;
	spall::Resource<spall::IDevice> m_Device;
	spall::Resource<spall::ISwapChain> m_SwapChain;

	spall::Resource<spall::ITexture2D> m_Output;
	spall::Resource<spall::ITextureView> m_OutputView;
	spall::Resource<spall::ITexture2D> m_Accumulation;
	spall::Resource<spall::ITextureView> m_AccumulationView;

	SceneResources m_SceneResources;
	spall::Resource<spall::IBuffer> m_Constants;

	spall::Resource<spall::IShader> m_RayGenerationShader;
	spall::Resource<spall::IShader> m_MissShader;
	spall::Resource<spall::IShader> m_ClosestHitShader;
	spall::Resource<spall::IShader> m_IntersectionShader;
	spall::Resource<spall::IResourceSetLayout> m_ResourceSetLayout;
	spall::Resource<spall::IResourceSet> m_ResourceSet;
	spall::Resource<spall::IPipeline> m_Pipeline;

	std::vector<spall::Resource<spall::ICommandList>> m_CommandLists;
	std::uint32_t m_FrameIndex = 0;
};
