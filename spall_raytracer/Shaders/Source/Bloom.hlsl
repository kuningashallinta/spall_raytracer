// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#define GroupSize 8

#ifdef __spirv__
	[[vk::combinedImageSampler]] [[vk::binding(0)]]
#endif
Texture2D<float4> Source : register(t0);

#ifdef __spirv__
	[[vk::combinedImageSampler]] [[vk::binding(0)]]
#endif
SamplerState Sampler : register(s0);

#ifdef __spirv__
	[[vk::image_format("rgba16f")]] [[vk::binding(1)]]
#endif
RWTexture2D<float4> Target : register(u1);

struct PushConstants
{
	float2 TexelSize;
	float Threshold;
	float Knee;
	float Radius;
};

#ifdef __spirv__
	[[vk::push_constant]] ConstantBuffer<PushConstants> Push;
#else
	ConstantBuffer<PushConstants> Push : register(b13);
#endif

static float3 knee(
	float3 color)
{
	const float shoulder = Push.Threshold * Push.Knee;
	const float brightness = max(color.r, max(color.g, color.b));

	float soft = clamp((brightness - Push.Threshold) + shoulder, 0.0f, 2.0f * shoulder);
	soft = (soft * soft) / max(4.0f * shoulder, 1e-4f);

	return color * (max(soft, brightness - Push.Threshold) / max(brightness, 1e-4f));
}

static float3 box(
	float2 uv,
	float2 texel)
{
	const float3 a = Source.SampleLevel(Sampler, uv + (texel * float2(-2.0f, -2.0f)), 0.0f).rgb;
	const float3 b = Source.SampleLevel(Sampler, uv + (texel * float2(0.0f, -2.0f)), 0.0f).rgb;
	const float3 c = Source.SampleLevel(Sampler, uv + (texel * float2(2.0f, -2.0f)), 0.0f).rgb;
	const float3 d = Source.SampleLevel(Sampler, uv + (texel * float2(-1.0f, -1.0f)), 0.0f).rgb;
	const float3 e = Source.SampleLevel(Sampler, uv + (texel * float2(1.0f, -1.0f)), 0.0f).rgb;
	const float3 f = Source.SampleLevel(Sampler, uv + (texel * float2(-2.0f, 0.0f)), 0.0f).rgb;
	const float3 g = Source.SampleLevel(Sampler, uv, 0.0f).rgb;
	const float3 h = Source.SampleLevel(Sampler, uv + (texel * float2(2.0f, 0.0f)), 0.0f).rgb;
	const float3 i = Source.SampleLevel(Sampler, uv + (texel * float2(-1.0f, 1.0f)), 0.0f).rgb;
	const float3 j = Source.SampleLevel(Sampler, uv + (texel * float2(1.0f, 1.0f)), 0.0f).rgb;
	const float3 k = Source.SampleLevel(Sampler, uv + (texel * float2(-2.0f, 2.0f)), 0.0f).rgb;
	const float3 l = Source.SampleLevel(Sampler, uv + (texel * float2(0.0f, 2.0f)), 0.0f).rgb;
	const float3 m = Source.SampleLevel(Sampler, uv + (texel * float2(2.0f, 2.0f)), 0.0f).rgb;

	float3 result = (d + e + i + j) * 0.125f;
	result += (a + b + g + f) * 0.03125f;
	result += (b + c + h + g) * 0.03125f;
	result += (f + g + l + k) * 0.03125f;
	result += (g + h + m + l) * 0.03125f;

	return result;
}

static float3 tent(
	float2 uv,
	float2 texel)
{
	float3 result = Source.SampleLevel(Sampler, uv + (texel * float2(-1.0f, 1.0f)), 0.0f).rgb;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(0.0f, 1.0f)), 0.0f).rgb * 2.0f;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(1.0f, 1.0f)), 0.0f).rgb;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(-1.0f, 0.0f)), 0.0f).rgb * 2.0f;
	result += Source.SampleLevel(Sampler, uv, 0.0f).rgb * 4.0f;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(1.0f, 0.0f)), 0.0f).rgb * 2.0f;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(-1.0f, -1.0f)), 0.0f).rgb;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(0.0f, -1.0f)), 0.0f).rgb * 2.0f;
	result += Source.SampleLevel(Sampler, uv + (texel * float2(1.0f, -1.0f)), 0.0f).rgb;

	return result * (1.0f / 16.0f);
}

[numthreads(GroupSize, GroupSize, 1)]
void thresholdMain(
	uint3 id : SV_DispatchThreadID)
{
	uint2 size;
	Target.GetDimensions(size.x, size.y);

	if ((id.x >= size.x) || (id.y >= size.y))
	{
		return;
	}

	const int3 base = int3(int2(id.xy) * 2, 0);

	float4 sum = Source.Load(base);
	sum += Source.Load(base + int3(1, 0, 0));
	sum += Source.Load(base + int3(0, 1, 0));
	sum += Source.Load(base + int3(1, 1, 0));

	Target[id.xy] = float4(knee(sum.rgb / max(sum.a, 1.0f)), 1.0f);
}

[numthreads(GroupSize, GroupSize, 1)]
void downsampleMain(
	uint3 id : SV_DispatchThreadID)
{
	uint2 size;
	Target.GetDimensions(size.x, size.y);

	if ((id.x >= size.x) || (id.y >= size.y))
	{
		return;
	}

	const float2 uv = (float2(id.xy) + 0.5f) / float2(size);

	Target[id.xy] = float4(box(uv, Push.TexelSize), 1.0f);
}

[numthreads(GroupSize, GroupSize, 1)]
void upsampleMain(
	uint3 id : SV_DispatchThreadID)
{
	uint2 size;
	Target.GetDimensions(size.x, size.y);

	if ((id.x >= size.x) || (id.y >= size.y))
	{
		return;
	}

	const float2 uv = (float2(id.xy) + 0.5f) / float2(size);

	Target[id.xy] += float4(tent(uv, Push.TexelSize * Push.Radius), 0.0f);
}
