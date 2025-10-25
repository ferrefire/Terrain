#ifndef TERRAIN_FUCTIONS_INCLUDED
#define TERRAIN_FUCTIONS_INCLUDED

const int cascadeCount = 8;
const float baseDistance = 0.075;

layout(set = 0, binding = 1) uniform sampler2D heightmaps[cascadeCount];

#include "packing.glsl"
#include "noise.glsl"
#include "sampling.glsl"

vec4 TerrainValues(vec2 worldPosition)
{
	vec2 uv;
	vec4 heightData;

	for (int i = 0; i < cascadeCount; i++)
	{
		uv = worldPosition / 10000.0 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = baseDistance * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;

			heightData = texture(heightmaps[i], uv + 0.5).rgba;

			float height = unpackRG8ToFloat(heightData.xy);

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
	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
	float size = baseDistance * pow(2.0, i);
	if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
	{
		uv = uv / size;
		heightData = texture(heightmaps[i], uv + 0.5).rgba;
		float height = unpackRG8ToFloat(heightData.xy);
		vec3 terrainNormal = UnpackNormal(vec4(heightData.zw, 0.0, 0.0), 1.0).xzy;

		return (vec4(height - 0.5, terrainNormal));
	}

	return (vec4(-0.5, 0, 1, 0));
}

vec2 TerrainCascadeLod(vec2 worldPosition)
{
	vec2 uv;

	for (int i = 0; i < cascadeCount; i++)
	{
		uv = worldPosition / 10000.0 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = baseDistance * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;
			return vec2(i, max(abs(uv.x), abs(uv.y)));
		}
	}

	return (vec2(cascadeCount, 0));
}

#endif