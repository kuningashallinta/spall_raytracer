// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <src/Camera/Camera.h>

#include <glm/vec4.hpp>

struct FrameConstants
{
	View Camera;
	glm::vec4 LightDirection;
	glm::vec4 LightColor;
};
