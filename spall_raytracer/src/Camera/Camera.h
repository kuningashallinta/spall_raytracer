// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct FrameConstants
{
	glm::vec4 Origin;
	glm::vec4 Forward;
	glm::vec4 Right;
	glm::vec4 Up;
};

class Camera
{
public:
	void setPosition(const glm::vec3& position);
	void setTarget(const glm::vec3& target);
	void setFieldOfView(float degrees);

	FrameConstants constants(float aspectRatio) const;

private:
	glm::vec3 m_Position = {0.0f, 0.0f, -1.0f};
	glm::vec3 m_Target = {0.0f, 0.0f, 0.0f};
	float m_FieldOfView = 45.0f;
};
