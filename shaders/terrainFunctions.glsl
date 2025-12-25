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

float BlendSample(sampler2D tex, vec3 uv, float texelSize)
{
	float offset = texelSize * 0.5;

	vec2 s0 = texture(tex, uv.xz + vec2(-offset)).rg;
	float v0 = (uv.y <= s0.g ? s0.r : 1.0);

	vec2 s1 = texture(tex, uv.xz + vec2(offset, -offset)).rg;
	float v1 = (uv.y <= s1.g ? s1.r : 1.0);

	vec2 s2 = texture(tex, uv.xz + vec2(-offset, offset)).rg;
	float v2 = (uv.y <= s2.g ? s2.r : 1.0);

	vec2 s3 = texture(tex, uv.xz + vec2(offset)).rg;
	float v3 = (uv.y <= s3.g ? s3.r : 1.0);

	return ((v0 + v1 + v2 + v3) * 0.25);
}

const float shadowTexelSize = 1.0 / 512.0;
const float maxDisUV = sqrt(2.0);

float TerrainShadow(vec3 worldPosition)
{
	vec2 uv = vec2(0.0);
	vec2 viewUV = vec2(0.0);
	float result = 1.0;
	float blend = 0.0;
	float heightUV = worldPosition.y / 5000.0;

	for (int i = 2; i >= 0; i--)
	{
		uv = worldPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		//viewUV = variables.viewPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		//viewUV = (viewUV * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			vec3 sampleUV = vec3(uv.x, heightUV, uv.y);
			float value = 1.0 - BlendSample(shadowmaps[i], sampleUV + 0.5, shadowTexelSize);
			//if (abs(uv.x) > 0.3 || abs(uv.y) > 0.3) {value *= 1.0 - (max(abs(uv.x), abs(uv.y)) - 0.3) / 0.2;}

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

float TerrainShadowLod(vec3 worldPosition, int lod)
{
	vec2 uv = vec2(0.0);
	float heightUV = worldPosition.y / 5000.0;
	float result = 1.0;
	float blend = 0.0;
	lod = clamp(lod, 0, 2);
	int i = lod;
	//for (int i = lod; i <= 2; i++)
	{
		uv = worldPosition.xz * 0.0001 - (variables.shadowmapOffsets[i].xz - variables.terrainOffset.xz);
		uv = (uv * 10000.0) / variables.shadowmapOffsets[i].w;

		if (abs(uv.x) < 0.5 && abs(uv.y) < 0.5)
		{
			//if (i < 2) {result = mix(BlendSample(shadowmaps[i], uv + 0.5, shadowTexelSize), result, blend);}
			//result = texture(shadowmaps[i], uv + 0.5).r;
			vec3 sampleUV = vec3(uv.x, heightUV, uv.y);
			result = BlendSample(shadowmaps[i], sampleUV + 0.5, shadowTexelSize);

			//if (abs(uv.x) < 0.4 && abs(uv.y) < 0.4) {break;}
			//else {blend = 1.0 - (max(abs(uv.x), abs(uv.y)) - 0.4) / 0.1;}
		}
	}

	return (result);
}

#endif