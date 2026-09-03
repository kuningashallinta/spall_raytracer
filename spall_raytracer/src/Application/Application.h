// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>

#include <src/Application/Window.h>
#include <src/Renderer/Renderer.h>
#include <src/Scene/Scene.h>

class Application
{
public:
	spall::Status run(void);

private:
	Scene m_Scene;
	Window m_Window;
	Renderer m_Renderer;
};
