// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#define GroupSize 8

#ifdef __spirv__
	[[vk::image_format("rgba32f")]] [[vk::binding(0)]]
#endif
RWTexture2D<float4> Accumulation : register(u0);

#ifdef __spirv__
	[[vk::combinedImageSampler]] [[vk::binding(1)]]
#endif
Texture2D<float4> Bloom : register(t1);

#ifdef __spirv__
	[[vk::combinedImageSampler]] [[vk::binding(1)]]
#endif
SamplerState Sampler : register(s1);

#ifdef __spirv__
	[[vk::image_format("rgba8")]] [[vk::binding(2)]]
#endif
RWTexture2D<float4> Output : register(u2);

struct PushConstants
{
	float Exposure;
	float Intensity;
};

#ifdef __spirv__
	[[vk::push_constant]] ConstantBuffer<PushConstants> Push;
#else
	ConstantBuffer<PushConstants> Push : register(b13);
#endif

static float3 aces(
	float3 color)
{
	return saturate((color * ((2.51f * color) + 0.03f)) / ((color * ((2.43f * color) + 0.59f)) + 0.14f));
}

static float3 linearToSrgb(
	float3 color)
{
	const float3 clamped = saturate(color);
	const float3 low = clamped * 12.92f;
	const float3 high = (1.055f * pow(clamped, 1.0f / 2.4f)) - 0.055f;

	return lerp(high, low, step(clamped, 0.0031308f));
}

[numthreads(GroupSize, GroupSize, 1)]
void compositeMain(
	uint3 id : SV_DispatchThreadID)
{
	uint2 size;
	Output.GetDimensions(size.x, size.y);

	if ((id.x >= size.x) || (id.y >= size.y))
	{
		return;
	}

	const float4 accumulated = Accumulation[id.xy];
	const float3 scene = accumulated.rgb / max(accumulated.a, 1.0f);

	const float2 uv = (float2(id.xy) + 0.5f) / float2(size);
	const float3 bloom = Bloom.SampleLevel(Sampler, uv, 0.0f).rgb;

	const float3 color = lerp(scene, bloom, Push.Intensity) * Push.Exposure;

	Output[id.xy] = float4(linearToSrgb(aces(color)), 1.0f);
}
