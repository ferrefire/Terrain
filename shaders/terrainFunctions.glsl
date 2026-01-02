#ifndef TERRAIN_FUCTIONS_INCLUDED
#define TERRAIN_FUCTIONS_INCLUDED

const float baseDistance = 0.075;
const float shadowResolution = 256.0;
const float shadowTexelSize = 1.0 / shadowResolution;

layout(set = 0, binding = 1) uniform sampler2D heightmaps[cascadeCount];
layout(set = 0, binding = 2) uniform sampler2D shadowmaps[3];
layout(set = 0, binding = 3) uniform sampler2D glillMap;
layout(set = 0, binding = 4) uniform sampler2D skyMap;

#include "packing.glsl"
#include "noise.glsl"
#include "sampling.glsl"
#include "atmosphere.glsl"

vec4 TerrainValues(vec2 worldPosition)
{
	vec2 uv;
	vec4 heightData;

	for (int i = 0; i < cascadeCount; i++)
	{
		uv = worldPosition * 0.0001 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = baseDistance * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;

			heightData = textureLod(heightmaps[i], uv + 0.5, 0).rgba;

			float height = unpackRG8ToFloat(heightData.xy);
			//float height = unpackRG16ToFloat(heightData.xy);

			//vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
			//vec3 terrainNormal = DerivativeToNormal(heightData.zw * 2.0 - 1.0);
			vec3 terrainNormal = UnpackNormal(vec4(heightData.zw, 0.0, 0.0), 1.0).xzy;

			return (vec4(height - 0.5, terrainNormal));
		}
	}

	return (vec4(-0.5, 0, 1, 0));
}

vec4 TerrainValuesLod(vec2 worldPosition, int targetLod)
{
	vec2 uv;
	vec4 heightData;

	targetLod = clamp(targetLod, 0, cascadeCount - 1);
	int i = targetLod;
	uv = worldPosition * 0.0001 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
	float size = baseDistance * pow(2.0, i);
	if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
	{
		uv = uv / size;
		heightData = textureLod(heightmaps[i], uv + 0.5, 0).rgba;
		float height = unpackRG8ToFloat(heightData.xy);
		//float height = unpackRG16ToFloat(heightData.xy);
		vec3 terrainNormal = UnpackNormal(vec4(heightData.zw, 0.0, 0.0), 1.0).xzy;

		return (vec4(height - 0.5, terrainNormal));
	}

	return (vec4(-0.5, 0, 1, 0));
}

float TerrainCascadeLod(vec2 worldPosition)
{
	vec2 uv;

	for (int i = 0; i < cascadeCount; i++)
	{
		uv = worldPosition * 0.0001 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = baseDistance * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;
			//return vec2(i, max(abs(uv.x), abs(uv.y)));
			return (float(i) + max(abs(uv.x), abs(uv.y)) * 2.0);
		}
	}

	//return (vec2(cascadeCount, 0));
	return (cascadeCount);
}

float BlendSample(sampler2D tex, vec3 uv, float texelSize)
{
	const float offset = texelSize * 0.5;

	vec2 s0 = texture(tex, uv.xz + vec2(-offset)).rg;
	//float d0 = clamp((uv.y - s0.g) * 5000.0, 0.0, 50.0) * 0.02;
	//float v0 = mix(s0.r, 1.0, d0);
	float v0 = (uv.y <= s0.g ? s0.r : 1.0);

	vec2 s1 = texture(tex, uv.xz + vec2(offset, -offset)).rg;
	//float d1 = clamp((uv.y - s1.g) * 5000.0, 0.0, 50.0) * 0.02;
	//float v1 = mix(s1.r, 1.0, d1);
	float v1 = (uv.y <= s1.g ? s1.r : 1.0);

	vec2 s2 = texture(tex, uv.xz + vec2(-offset, offset)).rg;
	//float d2 = clamp((uv.y - s2.g) * 5000.0, 0.0, 50.0) * 0.02;
	//float v2 = mix(s2.r, 1.0, d2);
	float v2 = (uv.y <= s2.g ? s2.r : 1.0);

	vec2 s3 = texture(tex, uv.xz + vec2(offset)).rg;
	//float d3 = clamp((uv.y - s3.g) * 5000.0, 0.0, 50.0) * 0.02;
	//float v3 = mix(s3.r, 1.0, d3);
	float v3 = (uv.y <= s3.g ? s3.r : 1.0);

	return ((v0 + v1 + v2 + v3) * 0.25);
}

const float maxDisUV = sqrt(2.0);
const float maxHeightMult = 1.0 / 5000.0;

float TerrainShadow(vec3 worldPosition)
{
	vec2 uv = vec2(0.0);
	vec2 viewUV = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	float heightUV = worldPosition.y * maxHeightMult;
	if (heightUV <= -0.5) {heightUV = -0.6;}

	for (int i = 2; i >= 0; i--)
	{
		uv = worldPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			vec3 sampleUV = vec3(uv.x, heightUV, uv.y) + 0.5;
			float value = 1.0 - BlendSample(shadowmaps[i], sampleUV, shadowTexelSize);
			
			if (i < 2)
			{
				viewUV = variables.viewPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
				viewUV = (viewUV * 10000.0) / variables.shadowmapOffsets[i].w;
				float uvDis = max(abs(uv.x - viewUV.x), abs(uv.y - viewUV.y));
				if (i > 0) {uvDis -= (variables.shadowmapOffsets[i - 1].w / variables.shadowmapOffsets[i].w) * 0.5;}
				uvDis = clamp(uvDis * 2.0, 0.0, 1.0);
				float valPower = clamp(uvDis - 0.3, 0.0, 0.3) / 0.3;
				value *= 1.0 - valPower;
			}

			result -= value;

			if (result <= 0.0) {break;}
		}
	}

	return (clamp(result, 0.0, 1.0));
}

float TerrainShadow(vec3 worldPosition, int lod, bool blended)
{
	vec2 uv = vec2(0.0);
	vec2 viewUV = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	float heightUV = worldPosition.y * maxHeightMult;
	if (heightUV <= -0.5) {heightUV = -0.6;}
	lod = clamp(lod, 0, 2);

	for (int i = 2; i >= lod; i--)
	{
		uv = worldPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			vec3 sampleUV = vec3(uv.x, heightUV, uv.y) + 0.5;
			float value = 0.0;

			if (blended)
			{
				value = 1.0 - BlendSample(shadowmaps[i], sampleUV, shadowTexelSize);
			}
			else
			{
				vec2 s0 = texture(shadowmaps[i], sampleUV.xz).rg;
				value = 1.0 - (sampleUV.y <= s0.g ? s0.r : 1.0);
			}
			
			if (i < 2)
			{
				viewUV = variables.viewPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
				viewUV = (viewUV * 10000.0) / variables.shadowmapOffsets[i].w;
				float uvDis = max(abs(uv.x - viewUV.x), abs(uv.y - viewUV.y));
				if (i > 0) {uvDis -= (variables.shadowmapOffsets[i - 1].w / variables.shadowmapOffsets[i].w) * 0.5;}
				uvDis = clamp(uvDis * 2.0, 0.0, 1.0);
				float valPower = clamp(uvDis - 0.3, 0.0, 0.3) / 0.3;
				value *= 1.0 - valPower;
			}

			result -= value;

			if (result <= 0.0) {break;}
		}
	}

	return (clamp(result, 0.0, 1.0));
}

vec2 TerrainShadowValue(vec3 worldPosition, int lod)
{
	vec2 uv = vec2(0.0);
	vec2 viewUV = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	float heightUV = worldPosition.y * maxHeightMult;
	int i = clamp(lod, 0, 2);

	uv = worldPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
	uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

	if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
	{
		vec3 sampleUV = vec3(uv.x, heightUV, uv.y) + 0.5;
		vec2 s0 = texture(shadowmaps[i], sampleUV.xz).rg;
		return (s0);
		//value = 1.0 - (sampleUV.y <= s0.g ? s0.r : 1.0);

		//result -= value;

		//if (result <= 0.0) {break;}
	}

	return (vec2(0.0));
}

/*float TerrainIllumination(vec2 worldPosition)
{
	float result = 1.0;

	vec2 uv = worldPosition * 0.0001 - (variables.glillOffsets.xz - variables.terrainOffset.xz);
	uv = (uv * 10000.0) / 10000.0;

	if (max(abs(uv.x), abs(uv.y)) <= 0.5)
	{
		result = texture(glillMap, uv + 0.5).r;
	}

	return (result);
}*/

vec4 TerrainOcclusion(vec3 worldPosition, vec3 worldDirection)
{
	float result = 1.0;

	vec2 uv = worldPosition.xz * 0.0001 - (variables.glillOffsets.xz - variables.terrainOffset.xz);
	uv = (uv * 10000.0) / 10000.0;

	if (max(abs(uv.x), abs(uv.y)) <= 0.5)
	{
		vec4 value = texture(glillMap, uv + 0.5);
		result = value.a;
		worldDirection = normalize(value.xyz);
	}

	vec3 skyWorldPosition = ((worldPosition + vec3(0.0, 2500.0 + (variables.terrainOffset.y * 10000.0), 0.0)) * cameraScale) + vec3(0.0, bottomRadius, 0.0);
	//vec3 skyWorldPosition = ((variables.viewPosition.xyz + vec3(0.0, 2500.0 + (variables.terrainOffset.y * 10000.0), 0.0)) * cameraScale) + vec3(0.0, bottomRadius, 0.0);
	vec3 up = normalize(skyWorldPosition);
	float viewAngle = acos(dot(worldDirection, up));
	float viewHeight = length(skyWorldPosition);

	float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
	bool groundIntersect = IntersectSphere(skyWorldPosition, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

	vec2 skyUV = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

	vec3 skyColor = texture(skyMap, skyUV).rgb * 300;

	return (vec4(skyColor, result));
}

vec3 TerrainIllumination(vec3 worldPosition, vec3 worldDirection)
{
	vec3 skyWorldPosition = ((worldPosition + vec3(0.0, 2500.0 + (variables.terrainOffset.y * 10000.0), 0.0)) * cameraScale) + vec3(0.0, bottomRadius, 0.0);
	//vec3 skyWorldPosition = ((variables.viewPosition.xyz + vec3(0.0, 2500.0 + (variables.terrainOffset.y * 10000.0), 0.0)) * cameraScale) + vec3(0.0, bottomRadius, 0.0);
	vec3 up = normalize(skyWorldPosition);
	float viewAngle = acos(dot(worldDirection, up));
	float viewHeight = length(skyWorldPosition);

	float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
	bool groundIntersect = IntersectSphere(skyWorldPosition, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

	vec2 skyUV = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

	vec3 skyColor = texture(skyMap, skyUV).rgb * 300;

	return (skyColor);
}

#endif