// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Camera/Camera.h>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

void Camera::setPosition(
	const glm::vec3& position)
{
	m_Position = position;
}

void Camera::setTarget(
	const glm::vec3& target)
{
	m_Target = target;
}

void Camera::setFieldOfView(
	float degrees)
{
	m_FieldOfView = degrees;
}

View Camera::view(
	float aspectRatio) const
{
	const glm::vec3 worldUp = {0.0f, 1.0f, 0.0f};

	const glm::vec3 forward = glm::normalize(m_Target - m_Position);
	const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
	const glm::vec3 up = glm::cross(right, forward);

	View view = {};
	view.Origin = glm::vec4(m_Position, glm::tan(glm::radians(m_FieldOfView) * 0.5f));
	view.Forward = glm::vec4(forward, aspectRatio);
	view.Right = glm::vec4(right, 0.0f);
	view.Up = glm::vec4(up, 0.0f);

	return view;
}
