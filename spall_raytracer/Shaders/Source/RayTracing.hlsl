// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#define MaterialLambertian 0
#define MaterialMirror 1
#define MaxBounces 2

struct Vertex
{
	float3 Position;
	float Padding;
};

struct MaterialRecord
{
	float4 Albedo;
	uint Type;
	uint FirstVertex;
	uint2 Unused;
};

struct RayPayload
{
	float3 Radiance;
	float3 Attenuation;
	float3 ScatterOrigin;
	float3 ScatterDirection;
	uint Scatter;
};

struct ShadowPayload
{
	uint Occluded;
};

struct SphereAttributes
{
	float3 Normal;
};

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
	float4 LightDirection;
	float4 LightColor;
};

RWStructuredBuffer<MaterialRecord> Materials : register(u3);
RWStructuredBuffer<Vertex> Vertices : register(u4);

static float3 linearToSrgb(
	float3 color)
{
	const float3 clamped = saturate(color);
	const float3 low = clamped * 12.92f;
	const float3 high = (1.055f * pow(clamped, 1.0f / 2.4f)) - 0.055f;

	return lerp(high, low, step(clamped, 0.0031308f));
}

static float3 worldNormal(
	float3 objectNormal)
{
	return normalize(mul(objectNormal, (float3x3)WorldToObject3x4()));
}

static void shade(
	inout RayPayload payload,
	float3 normal,
	MaterialRecord material)
{
	const float3 position = WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());

	if (dot(normal, WorldRayDirection()) > 0.0f)
	{
		normal = -normal;
	}

	const float3 offset = position + (normal * 0.001f);

	if (material.Type == MaterialMirror)
	{
		payload.Radiance = float3(0.0f, 0.0f, 0.0f);
		payload.Attenuation = material.Albedo.rgb;
		payload.ScatterOrigin = offset;
		payload.ScatterDirection = reflect(WorldRayDirection(), normal);
		payload.Scatter = 1;

		return;
	}

	const float lambert = saturate(dot(normal, LightDirection.xyz));
	float visibility = 0.0f;

	if (lambert > 0.0f)
	{
		ShadowPayload shadow;
		shadow.Occluded = 1;

		RayDesc shadowRay;
		shadowRay.Origin = offset;
		shadowRay.Direction = LightDirection.xyz;
		shadowRay.TMin = 0.0f;
		shadowRay.TMax = 1000.0f;

		TraceRay(
			Scene,
			RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
			0xFF,
			0,
			0,
			1,
			shadowRay,
			shadow);

		visibility = (shadow.Occluded == 0) ? 1.0f : 0.0f;
	}

	payload.Radiance = material.Albedo.rgb * ((LightColor.rgb * lambert * visibility) + LightColor.w);
	payload.Attenuation = float3(0.0f, 0.0f, 0.0f);
	payload.Scatter = 0;
}

[shader("raygeneration")]
void rayGenMain(void)
{
	const uint2 pixel = DispatchRaysIndex().xy;
	const float2 uv = (float2(pixel) + 0.5f) / float2(DispatchRaysDimensions().xy);
	const float2 ndc = float2((uv.x * 2.0f) - 1.0f, 1.0f - (uv.y * 2.0f));

	RayDesc ray;
	ray.Origin = Origin.xyz;
	ray.Direction = normalize(Forward.xyz
		+ (Right.xyz * ndc.x * Origin.w * Forward.w)
		+ (Up.xyz * ndc.y * Origin.w));
	ray.TMin = 0.001f;
	ray.TMax = 1000.0f;

	float3 radiance = float3(0.0f, 0.0f, 0.0f);
	float3 throughput = float3(1.0f, 1.0f, 1.0f);

	for (uint bounce = 0; bounce < MaxBounces; ++bounce)
	{
		RayPayload payload;
		payload.Radiance = float3(0.0f, 0.0f, 0.0f);
		payload.Attenuation = float3(0.0f, 0.0f, 0.0f);
		payload.ScatterOrigin = float3(0.0f, 0.0f, 0.0f);
		payload.ScatterDirection = float3(0.0f, 0.0f, 0.0f);
		payload.Scatter = 0;

		TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

		radiance += throughput * payload.Radiance;

		if (payload.Scatter == 0)
		{
			break;
		}

		throughput *= payload.Attenuation;
		ray.Origin = payload.ScatterOrigin;
		ray.Direction = payload.ScatterDirection;
	}

	Output[pixel] = float4(linearToSrgb(radiance), 1.0f);
}

[shader("miss")]
void missMain(
	inout RayPayload payload)
{
	const float height = saturate((WorldRayDirection().y * 0.5f) + 0.5f);

	payload.Radiance = lerp(float3(0.62f, 0.70f, 0.86f), float3(0.09f, 0.16f, 0.34f), height);
	payload.Scatter = 0;
}

[shader("miss")]
void shadowMissMain(
	inout ShadowPayload payload)
{
	payload.Occluded = 0;
}

[shader("closesthit")]
void closestHitMain(
	inout RayPayload payload,
	in BuiltInTriangleIntersectionAttributes attributes)
{
	const MaterialRecord material = Materials[InstanceID()];
	const uint base = material.FirstVertex + (PrimitiveIndex() * 3);

	const float3 a = Vertices[base + 0].Position;
	const float3 b = Vertices[base + 1].Position;
	const float3 c = Vertices[base + 2].Position;

	shade(payload, worldNormal(cross(b - a, c - a)), material);
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

	if (t < RayTMin())
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
	shade(payload, worldNormal(attributes.Normal), Materials[InstanceID()]);
}
