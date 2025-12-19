#ifndef TERRAIN_FUCTIONS_INCLUDED
#define TERRAIN_FUCTIONS_INCLUDED

const float baseDistance = 0.075;

layout(set = 0, binding = 1) uniform sampler2D heightmaps[cascadeCount];
layout(set = 0, binding = 2) uniform sampler2D shadowmaps[3];

#include "packing.glsl"
#include "noise.glsl"
#include "sampling.glsl"

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

float BlendSample(sampler2D tex, vec2 uv, float texelSize)
{
	float offset = texelSize * 0.5;

	float s0 = texture(tex, uv + vec2(-offset)).r;
	float s1 = texture(tex, uv + vec2(offset, -offset)).r;
	float s2 = texture(tex, uv + vec2(-offset, offset)).r;
	float s3 = texture(tex, uv + vec2(offset)).r;

	return ((s0 + s1 + s2 + s3) * 0.25);
}

const float shadowTexelSize = 1.0 / 256.0;

float TerrainShadow(vec2 worldPosition)
{
	vec2 uv = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	for (int i = 2; i >= 0; i--)
	{
		uv = worldPosition * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			//if (i < 2) {result = mix(BlendSample(shadowmaps[i], uv + 0.5, shadowTexelSize), result, blend);}
			//else {result = mix(texture(shadowmaps[i], uv + 0.5).r, result, blend);}
			//result = mix(BlendSample(shadowmaps[i], uv + 0.5, shadowTexelSize), result, blend);
			float value = 1.0 - BlendSample(shadowmaps[i], uv + 0.5, shadowTexelSize);
			if (abs(uv.x) > 0.3 || abs(uv.y) > 0.3) {value *= 1.0 - (max(abs(uv.x), abs(uv.y)) - 0.3) / 0.2;}
			result -= value;

			if (result <= 0.0) {break;}

			//if (abs(uv.x) < 0.4 && abs(uv.y) < 0.4) {blend = 0.0;}
			//else {blend = 1.0 - (max(abs(uv.x), abs(uv.y)) - 0.4) / 0.1;}
		}
	}

	return (clamp(result, 0.0, 1.0));
}

float TerrainShadowLod(vec2 worldPosition, int lod)
{
	vec2 uv = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	lod = clamp(lod, 0, 2);
	for (int i = lod; i <= 2; i++)
	{
		uv = worldPosition * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			//if (i < 2) {result = mix(BlendSample(shadowmaps[i], uv + 0.5, shadowTexelSize), result, blend);}
			result = mix(texture(shadowmaps[i], uv + 0.5).r, result, blend);

			if (abs(uv.x) < 0.4 && abs(uv.y) < 0.4) {break;}
			else {blend = 1.0 - (max(abs(uv.x), abs(uv.y)) - 0.4) / 0.1;}
		}
	}

	return (result);
}

#endif