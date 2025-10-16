#ifndef LIGHTING_INCLUDED
#define LIGHTING_INCLUDED

#include "random.glsl"

struct PBRInput
{
	vec3 N;
	vec3 V;
	vec3 L;

	vec3 albedo;
	float roughness;
	float metallic;

	vec3 lightColor;
};

float Saturate(float x)
{
	return (clamp(x, 0.0, 1.0));
}

vec3 Saturate(vec3 vec)
{
	return (clamp(vec, 0.0, 1.0));
}

vec3 Fresnel(float cosTheta, vec3 F0)
{
	return (F0 + (1.0 - F0) * pow(1.0 - Saturate(cosTheta), 5.0));
}

float DGGX(float NH, float alpha)
{
	float a2 = alpha * alpha;
	float d = (NH * NH) * (a2 - 1.0) + 1.0;
	float D = (a2 / (3.14159265 * d * d + 1e-7));

	return (D);
}

float SchlickGGX(float NX, float k)
{
	float G = NX / (NX * (1.0 - k) + k + 1e-7);

	return (G);
}

float Smith(float NV, float NL, float k)
{
	return (SchlickGGX(NV, k) * SchlickGGX(NL, k));
}

vec3 PBRLighting(PBRInput data)
{
	vec3 H = normalize(data.V + data.L);
	float NL = Saturate(dot(data.N, data.L));
	float NV = Saturate(dot(data.N, data.V));
	float NH = Saturate(dot(data.N, H));
	float VH = Saturate(dot(data.V, H));

	//if (NL <= 0.0 || NV <= 0.0) return (vec3(0.0));

	vec3 F0 = mix(vec3(0.04), data.albedo, data.metallic);

	float alpha = max(data.roughness * data.roughness, 0.001);
	float D = DGGX(NH, alpha);

	float k = data.roughness + 1.0;
	k = (k * k) * 0.125;
	float G = Smith(NV, NL, k);
	
	vec3 F = Fresnel(VH, F0);

	float denominator = max(4.0 * NV * NL, 1e-7);
	vec3 specular = (D * G * F) / denominator;

	vec3 kd = (1.0 - F) * (1.0 - data.metallic);
	vec3 diffuse = kd * data.albedo / 3.14159265;

	vec3 lo = (diffuse + specular) * data.lightColor * NL;

	return (lo);
}

vec3 DiffuseLighting(vec3 normal, vec3 lightDirection, vec3 color)
{
	float product = dot(normal, lightDirection);
	float intensity = max(0.1, product);
	vec3 result = color * intensity;

	return (result);
}

vec3 ToNonLinear(vec3 linearColor)
{
	vec3 cutoff = step(vec3(0.0031308), linearColor);
	vec3 lower = linearColor * 12.92;
	vec3 higher = 1.055 * pow(linearColor, vec3(1.0 / 2.4)) - 0.055;

	return (mix(lower, higher, cutoff));
}

vec3 Normalize(vec3 vec)
{
	float total = abs(vec.x) + abs(vec.y) + abs(vec.z);
	vec3 result = vec / total;

	return (vec);
}

vec3 FaceColor()
{
	int primitiveID = gl_PrimitiveID + 1;
	float r = rand(vec2(primitiveID, primitiveID % 3));
	float g = rand(vec2(r, r * primitiveID));
	float b = rand(vec2(r, g));
	vec3 color = vec3(r, g, b);

	return (color);
}

#endif