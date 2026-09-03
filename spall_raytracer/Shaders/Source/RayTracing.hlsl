// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

RaytracingAccelerationStructure Scene : register(t0);

#ifdef __spirv__
	[[vk::image_format("rgba8")]]
#endif
RWTexture2D<float4> Output : register(u1);

cbuffer FrameConstants : register(b2)
{
	float4 Origin;
	float4 Forward;
	float4 Right;
	float4 Up;
};

struct RayPayload
{
	float3 Color;
};

struct SphereAttributes
{
	float3 Normal;
};

static float3 linearToSrgb(
	float3 color)
{
	const float3 clamped = saturate(color);
	const float3 low = clamped * 12.92f;
	const float3 high = (1.055f * pow(clamped, 1.0f / 2.4f)) - 0.055f;

	return lerp(high, low, step(clamped, 0.0031308f));
}

[shader("raygeneration")]
void rayGenMain(void)
{
	const uint2 pixel = DispatchRaysIndex().xy;
	const float2 uv = (float2(pixel) + 0.5f) / float2(DispatchRaysDimensions().xy);
	const float2 ndc = float2((uv.x * 2.0f) - 1.0f, 1.0f - (uv.y * 2.0f));

	RayDesc ray;
	ray.Origin = Origin.xyz;
	ray.Direction = normalize(Forward.xyz + (Right.xyz * ndc.x * Origin.w * Forward.w) + (Up.xyz * ndc.y * Origin.w));
	ray.TMin = 0.001f;
	ray.TMax = 1000.0f;

	RayPayload payload;
	payload.Color = float3(0.0f, 0.0f, 0.0f);

	TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

	Output[pixel] = float4(linearToSrgb(payload.Color), 1.0f);
}

[shader("miss")]
void missMain(
	inout RayPayload payload)
{
	const float height = saturate((WorldRayDirection().y * 0.5f) + 0.5f);

	payload.Color = lerp(float3(0.62f, 0.70f, 0.86f), float3(0.09f, 0.16f, 0.34f), height);
}

[shader("closesthit")]
void closestHitMain(
	inout RayPayload payload,
	in BuiltInTriangleIntersectionAttributes attributes)
{
	payload.Color = (InstanceID() == 0)
		? float3(0.85f, 0.34f, 0.14f)
		: float3(0.52f, 0.54f, 0.58f);
}

[shader("intersection")]
void intersectionMain(void)
{
	const float3 origin = ObjectRayOrigin();
	const float3 direction = ObjectRayDirection();

	const float a = dot(direction, direction);
	const float b = 2.0f * dot(origin, direction);
	const float c = dot(origin, origin) - 0.25f;
	const float discriminant = (b * b) - (4.0f * a * c);

	if (discriminant < 0.0f)
	{
		return;
	}

	const float root = sqrt(discriminant);
	float t = (-b - root) / (2.0f * a);

	if (t < RayTMin())
	{
		t = (-b + root) / (2.0f * a);
	}

	if ((t < RayTMin()) || (t > RayTCurrent()))
	{
		return;
	}

	SphereAttributes attributes;
	attributes.Normal = normalize(origin + (t * direction));

	ReportHit(t, 0, attributes);
}

[shader("closesthit")]
void proceduralClosestHitMain(
	inout RayPayload payload,
	in SphereAttributes attributes)
{
	payload.Color = float3(0.22f, 0.52f, 0.86f);
}
