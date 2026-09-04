// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Enums/ResourceEnums.h>

#include <cstdint>

inline constexpr spall::RenderBackendType BackendType = spall::RenderBackendType::D3D12;

inline constexpr spall::Format OutputFormat = spall::Format::RGBA8;
inline constexpr spall::Format AccumulationFormat = spall::Format::RGBA32Float;
inline constexpr spall::Format BloomFormat = spall::Format::RGBA16Float;

inline constexpr std::uint32_t WindowWidth = 1920;
inline constexpr std::uint32_t WindowHeight = 1080;

inline constexpr std::uint32_t BloomLevels = 5;
inline constexpr std::uint32_t GroupSize = 8;

inline constexpr std::uint32_t SceneBinding = 0;
inline constexpr std::uint32_t ConstantsBinding = 1;
inline constexpr std::uint32_t MaterialBinding = 2;
inline constexpr std::uint32_t VertexBinding = 3;
inline constexpr std::uint32_t InstanceBinding = 4;
inline constexpr std::uint32_t AccumulationBinding = 5;

inline constexpr std::uint32_t BloomSourceBinding = 0;
inline constexpr std::uint32_t BloomTargetBinding = 1;

inline constexpr std::uint32_t CompositeSceneBinding = 0;
inline constexpr std::uint32_t CompositeBloomBinding = 1;
inline constexpr std::uint32_t CompositeOutputBinding = 2;

inline constexpr std::uint32_t MaxPayloadSize = 64;
inline constexpr std::uint32_t MaxAttributeSize = 12;
inline constexpr std::uint32_t MaxRecursionDepth = 2;
