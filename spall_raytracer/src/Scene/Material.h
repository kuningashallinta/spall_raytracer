// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <glm/vec3.hpp>

#include <cstdint>

using MaterialIndex = std::uint32_t;

enum class MaterialType : std::uint32_t
{
	Lambertian,
	Mirror
};

struct Material
{
	glm::vec3 Albedo = {0.8f, 0.8f, 0.8f};
	MaterialType Type = MaterialType::Lambertian;
};

inline bool operator==(
	const Material& a,
	const Material& b)
{
	return (a.Albedo == b.Albedo) and (a.Type == b.Type);
}
