// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <src/Scene/Shape.h>

#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>

void Shape::setPosition(
	const glm::vec3& position)
{
	m_Position = position;
}

void Shape::setRotation(
	const glm::vec3& degrees)
{
	m_Rotation = degrees;
}

void Shape::setScale(
	const glm::vec3& scale)
{
	m_Scale = scale;
}

void Shape::setScale(
	float scale)
{
	m_Scale = glm::vec3(scale);
}

void Shape::setMaterial(
	MaterialIndex material)
{
	m_Material = material;
}

MaterialIndex Shape::material(
	void) const
{
	return m_Material;
}

void Shape::writeTransform(
	float (&transform)[12]) const
{
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), m_Position);

	matrix = glm::rotate(matrix, glm::radians(m_Rotation.y), {0.0f, 1.0f, 0.0f});
	matrix = glm::rotate(matrix, glm::radians(m_Rotation.x), {1.0f, 0.0f, 0.0f});
	matrix = glm::rotate(matrix, glm::radians(m_Rotation.z), {0.0f, 0.0f, 1.0f});
	matrix = glm::scale(matrix, m_Scale);

	for (int row = 0; row < 3; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			transform[(row * 4) + column] = matrix[column][row];
		}
	}
}
