// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Application/Application.h>

#include <src/Renderer/RendererConfig.h>

spall::Status Application::run(
	void)
{
	spall::Status status = m_Window.open(WindowWidth, WindowHeight, L"Spall Raytracer");

	if (status != spall::SUCCESS)
	{
		return status;
	}

	m_Scene.build();

	status = m_Renderer.initialize(m_Window.handle(), m_Scene);

	if (status != spall::SUCCESS)
	{
		return status;
	}

	while (not m_Window.closed())
	{
		m_Window.pump();

		status = m_Renderer.renderFrame();

		if (status != spall::SUCCESS)
		{
			break;
		}
	}

	m_Renderer.waitIdle();

	return status;
}
