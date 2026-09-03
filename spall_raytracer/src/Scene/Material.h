// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <glm/vec3.hpp>

#include <cstdint>

using MaterialIndex = std::uint32_t;

struct Material
{
	glm::vec3 Albedo = {0.8f, 0.8f, 0.8f};
	float Roughness = 1.0f;
	float Metallic = 0.0f;
	glm::vec3 Emission = {0.0f, 0.0f, 0.0f};
	float Transmission = 0.0f;
	float Ior = 1.5f;

	bool operator==(const Material&) const = default;
};
