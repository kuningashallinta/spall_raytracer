// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

inline constexpr spall::RenderBackendType BackendType = spall::RenderBackendType::D3D12;

inline constexpr spall::Format OutputFormat = spall::Format::RGBA8;

inline constexpr std::uint32_t WindowWidth = 1280;
inline constexpr std::uint32_t WindowHeight = 720;

inline constexpr std::uint32_t SceneBinding = 0;
inline constexpr std::uint32_t OutputBinding = 1;
inline constexpr std::uint32_t ConstantsBinding = 2;

inline constexpr std::uint32_t MaxPayloadSize = 16;
inline constexpr std::uint32_t MaxAttributeSize = 12;
inline constexpr std::uint32_t MaxRecursionDepth = 2;
