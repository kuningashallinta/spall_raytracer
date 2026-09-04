// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#define MaxBounces 8
#define Pi 3.14159265358979f

struct Vertex
{
	float3 Position;
	float Padding;
};

struct MaterialRecord
{
	float3 Albedo;
	float Roughness;
	float3 Emission;
	float Metallic;
	float Transmission;
	float Ior;
	uint2 Unused;
};

struct InstanceRecord
{
	uint MaterialIndex;
	uint FirstVertex;
	uint2 Unused;
};

struct RayPayload
{
	float3 Radiance;
	float3 Attenuation;
	float3 ScatterOrigin;
	float3 ScatterDirection;
	uint Seed;
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

cbuffer FrameConstants : register(b1)
{
	float4 Origin;
	float4 Forward;
	float4 Right;
	float4 Up;
	float4 LightDirection;
	float4 LightColor;
};

RWStructuredBuffer<MaterialRecord> Materials : register(u2);
RWStructuredBuffer<Vertex> Vertices : register(u3);
RWStructuredBuffer<InstanceRecord> Instances : register(u4);

#ifdef __spirv__
	[[vk::image_format("rgba32f")]]
#endif
RWTexture2D<float4> Accumulation : register(u5);

struct PushConstants
{
	uint FrameIndex;
};

#ifdef __spirv__
	[[vk::push_constant]] ConstantBuffer<PushConstants> Push;
#else
	ConstantBuffer<PushConstants> Push : register(b13);
#endif

static uint pcg(
	inout uint state)
{
	state = (state * 747796405u) + 2891336453u;

	const uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;

	return (word >> 22u) ^ word;
}

static uint seed(
	uint2 pixel,
	uint frame)
{
	uint state = (pixel.x * 73856093u) ^ (pixel.y * 19349663u) ^ (frame * 83492791u);

	return pcg(state);
}

static float random(
	inout uint state)
{
	return float(pcg(state)) * 2.3283064365386963e-10f;
}

static float radicalInverse(
	uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);

	return float(bits) * 2.3283064365386963e-10f;
}

static float halton3(
	uint index)
{
	float result = 0.0f;
	float fraction = 1.0f / 3.0f;

	while (index > 0u)
	{
		result += fraction * float(index % 3u);
		index /= 3u;
		fraction /= 3.0f;
	}

	return result;
}

static float3x3 basis(
	float3 normal)
{
	const float3 up = (abs(normal.z) < 0.999f) ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
	const float3 tangent = normalize(cross(up, normal));

	return float3x3(tangent, cross(normal, tangent), normal);
}

static float3 cosineHemisphere(
	float3 normal,
	inout uint state)
{
	const float angle = random(state) * (2.0f * Pi);
	const float radius = sqrt(random(state));

	const float3 local = float3(
		radius * cos(angle),
		radius * sin(angle),
		sqrt(saturate(1.0f - (radius * radius))));

	return normalize(mul(local, basis(normal)));
}

static float3 ggxHalf(
	float3 normal,
	float alpha,
	inout uint state)
{
	const float angle = random(state) * (2.0f * Pi);
	const float u = random(state);

	const float cosine = sqrt((1.0f - u) / (1.0f + (((alpha * alpha) - 1.0f) * u)));
	const float sine = sqrt(saturate(1.0f - (cosine * cosine)));

	const float3 local = float3(sine * cos(angle), sine * sin(angle), cosine);

	return normalize(mul(local, basis(normal)));
}

static float ggxDistribution(
	float cosine,
	float alpha)
{
	const float a2 = alpha * alpha;
	const float d = ((cosine * cosine) * (a2 - 1.0f)) + 1.0f;

	return a2 / max(Pi * d * d, 1e-7f);
}

static float smithG1(
	float cosine,
	float alpha)
{
	const float a2 = alpha * alpha;

	return (2.0f * cosine) / (cosine + sqrt(a2 + ((1.0f - a2) * cosine * cosine)));
}

static float3 fresnelSchlick(
	float3 f0,
	float cosine)
{
	return f0 + ((1.0f - f0) * pow(1.0f - cosine, 5.0f));
}

static float fresnelDielectric(
	float cosine,
	float eta)
{
	const float sine = (eta * eta) * (1.0f - (cosine * cosine));

	if (sine >= 1.0f)
	{
		return 1.0f;
	}

	const float transmitted = sqrt(1.0f - sine);

	const float s = ((eta * cosine) - transmitted) / ((eta * cosine) + transmitted);
	const float p = ((eta * transmitted) - cosine) / ((eta * transmitted) + cosine);

	return 0.5f * ((s * s) + (p * p));
}

static float3 worldNormal(
	float3 objectNormal)
{
	return normalize(mul(objectNormal, (float3x3)WorldToObject3x4()));
}

static float visibility(
	float3 origin,
	float3 direction)
{
	ShadowPayload shadow;
	shadow.Occluded = 1;

	RayDesc ray;
	ray.Origin = origin;
	ray.Direction = direction;
	ray.TMin = 0.0f;
	ray.TMax = 1000.0f;

	TraceRay(
		Scene,
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		0xFF,
		0,
		0,
		1,
		ray,
		shadow);

	return (shadow.Occluded == 0) ? 1.0f : 0.0f;
}

static void shade(
	inout RayPayload payload,
	float3 surfaceNormal,
	MaterialRecord material)
{
	const float3 direction = WorldRayDirection();
	const float3 view = -direction;
	const float3 position = WorldRayOrigin() + (RayTCurrent() * direction);

	const bool entering = (dot(surfaceNormal, direction) < 0.0f);
	const float3 normal = entering ? surfaceNormal : -surfaceNormal;
	const float alpha = max(material.Roughness * material.Roughness, 0.002f);

	payload.Radiance = material.Emission;
	payload.Attenuation = float3(1.0f, 1.0f, 1.0f);
	payload.Scatter = 1;

	if (random(payload.Seed) < material.Transmission)
	{
		const float3 microfacet = ggxHalf(normal, alpha, payload.Seed);
		const float eta = entering ? (1.0f / material.Ior) : material.Ior;
		const float reflectance = fresnelDielectric(saturate(dot(view, microfacet)), eta);

		float3 scatter = refract(direction, microfacet, eta);

		if ((random(payload.Seed) < reflectance) || (dot(scatter, scatter) < 0.5f))
		{
			scatter = reflect(direction, microfacet);
		}
		else
		{
			payload.Attenuation = material.Albedo;
		}

		payload.ScatterOrigin = position + (scatter * 0.002f);
		payload.ScatterDirection = normalize(scatter);

		return;
	}

	const float3 f0 = lerp(float3(0.04f, 0.04f, 0.04f), material.Albedo, material.Metallic);
	const float3 diffuse = material.Albedo * (1.0f - material.Metallic);

	const float ndotv = max(dot(normal, view), 1e-4f);
	const float ndotl = dot(normal, LightDirection.xyz);
	const float3 offset = position + (normal * 0.002f);

	if (ndotl > 0.0f)
	{
		const float shadow = visibility(offset, LightDirection.xyz);
		const float3 microfacet = normalize(LightDirection.xyz + view);

		const float3 specular = (fresnelSchlick(f0, saturate(dot(LightDirection.xyz, microfacet)))
			* ggxDistribution(saturate(dot(normal, microfacet)), alpha)
			* smithG1(ndotv, alpha)
			* smithG1(ndotl, alpha))
			/ (4.0f * ndotv);

		payload.Radiance += (((diffuse / Pi) * ndotl) + specular) * LightColor.rgb * shadow;
	}

	const float3 reflectance = fresnelSchlick(f0, ndotv);

	const float specularWeight = dot(reflectance, float3(1.0f, 1.0f, 1.0f));
	const float diffuseWeight = dot(diffuse, float3(1.0f, 1.0f, 1.0f));
	const float specularChance = specularWeight / max(specularWeight + diffuseWeight, 1e-4f);

	if (random(payload.Seed) < specularChance)
	{
		const float3 microfacet = ggxHalf(normal, alpha, payload.Seed);
		const float3 scatter = reflect(direction, microfacet);
		const float scatterCosine = dot(normal, scatter);

		if (scatterCosine <= 0.0f)
		{
			payload.Scatter = 0;

			return;
		}

		const float vdoth = saturate(dot(view, microfacet));
		const float ndoth = max(dot(normal, microfacet), 1e-4f);

		payload.Attenuation = (fresnelSchlick(f0, vdoth)
			* smithG1(ndotv, alpha)
			* smithG1(scatterCosine, alpha)
			* vdoth)
			/ (ndotv * ndoth * specularChance);

		payload.ScatterOrigin = offset;
		payload.ScatterDirection = scatter;

		return;
	}

	payload.Attenuation = diffuse / max(1.0f - specularChance, 1e-4f);
	payload.ScatterOrigin = offset;
	payload.ScatterDirection = cosineHemisphere(normal, payload.Seed);
}

[shader("raygeneration")]
void rayGenMain(void)
{
	const uint2 pixel = DispatchRaysIndex().xy;
	const float2 jitter = float2(radicalInverse(Push.FrameIndex + 1u), halton3(Push.FrameIndex + 1u));
	const float2 uv = (float2(pixel) + jitter) / float2(DispatchRaysDimensions().xy);
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
	uint state = seed(pixel, Push.FrameIndex);

	for (uint bounce = 0; bounce < MaxBounces; ++bounce)
	{
		RayPayload payload;
		payload.Radiance = float3(0.0f, 0.0f, 0.0f);
		payload.Attenuation = float3(0.0f, 0.0f, 0.0f);
		payload.ScatterOrigin = float3(0.0f, 0.0f, 0.0f);
		payload.ScatterDirection = float3(0.0f, 0.0f, 0.0f);
		payload.Seed = state;
		payload.Scatter = 0;

		TraceRay(Scene, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

		state = payload.Seed;
		radiance += throughput * payload.Radiance;

		if (payload.Scatter == 0)
		{
			break;
		}

		throughput *= payload.Attenuation;
		ray.Origin = payload.ScatterOrigin;
		ray.Direction = payload.ScatterDirection;
	}

	const float4 previous = (Push.FrameIndex == 0u) ? float4(0.0f, 0.0f, 0.0f, 0.0f) : Accumulation[pixel];

	Accumulation[pixel] = previous + float4(radiance, 1.0f);
}

[shader("miss")]
void missMain(
	inout RayPayload payload)
{
	const float height = saturate((WorldRayDirection().y * 0.5f) + 0.5f);

	payload.Radiance = lerp(float3(0.62f, 0.70f, 0.86f), float3(0.09f, 0.16f, 0.34f), height) * LightColor.w;
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
	const InstanceRecord instance = Instances[InstanceID()];
	const MaterialRecord material = Materials[instance.MaterialIndex];
	const uint base = instance.FirstVertex + (PrimitiveIndex() * 3);

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
	shade(payload, worldNormal(attributes.Normal), Materials[Instances[InstanceID()].MaterialIndex]);
}
