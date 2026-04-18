#ifndef TERRAIN_FUCTIONS_INCLUDED
#define TERRAIN_FUCTIONS_INCLUDED

#extension GL_EXT_nonuniform_qualifier : enable

const float baseDistance = 0.075;
const float shadowResolution = 256.0;
const float shadowTexelSize = 1.0 / shadowResolution;

layout(set = 0, binding = 1) uniform sampler2D heightmaps[cascadeCount];
layout(set = 0, binding = 2) uniform sampler2D terrainShadowMaps[3];
layout(set = 0, binding = 3) uniform sampler2D glillMaps[3];
layout(set = 0, binding = 4) uniform sampler2D skyMap;
layout(set = 0, binding = 6) uniform sampler2DShadow shadowMaps[3];
layout(set = 0, binding = 7, std140) uniform ShadowData
{
	uint enabled;
	float blend0Dis;
	float blend1Dis;
	uint test;
} shadowData;

#include "packing.glsl"
#include "sampling.glsl"
#include "atmosphere.glsl"
#include "functions.glsl"

vec4 TerrainValues(vec2 worldPosition, int startLod)
{
	vec2 uv;
	vec4 heightData;

	if (startLod >= cascadeCount - 1) {startLod = cascadeCount - 1;}

	for (int i = startLod; i < cascadeCount; i++)
	{
		uv = worldPosition * 0.0001 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = baseDistance * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;

			heightData = textureLod(heightmaps[nonuniformEXT(i)], uv + 0.5, 0).rgba;

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

vec4 TerrainValues(vec2 worldPosition)
{
	return (TerrainValues(worldPosition, 0));
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
		heightData = textureLod(heightmaps[nonuniformEXT(i)], uv + 0.5, 0).rgba;
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
	float d0 = clamp((uv.y - s0.g) * 5000.0, 0.0, 10.0) * 0.1;
	float v0 = mix(s0.r, 1.0, d0);
	//float v0 = (uv.y <= s0.g ? s0.r : 1.0);

	vec2 s1 = texture(tex, uv.xz + vec2(offset, -offset)).rg;
	float d1 = clamp((uv.y - s1.g) * 5000.0, 0.0, 10.0) * 0.1;
	float v1 = mix(s1.r, 1.0, d1);
	//float v1 = (uv.y <= s1.g ? s1.r : 1.0);

	vec2 s2 = texture(tex, uv.xz + vec2(-offset, offset)).rg;
	float d2 = clamp((uv.y - s2.g) * 5000.0, 0.0, 10.0) * 0.1;
	float v2 = mix(s2.r, 1.0, d2);
	//float v2 = (uv.y <= s2.g ? s2.r : 1.0);

	vec2 s3 = texture(tex, uv.xz + vec2(offset)).rg;
	float d3 = clamp((uv.y - s3.g) * 5000.0, 0.0, 10.0) * 0.1;
	float v3 = mix(s3.r, 1.0, d3);
	//float v3 = (uv.y <= s3.g ? s3.r : 1.0);

	return ((v0 + v1 + v2 + v3) * 0.25);
}

const float maxDisUV = sqrt(2.0);
const float maxHeightMult = 1.0 / maxHeight;

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
			float value = 1.0 - BlendSample(terrainShadowMaps[nonuniformEXT(i)], sampleUV, shadowTexelSize);
			
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
				value = 1.0 - BlendSample(terrainShadowMaps[nonuniformEXT(i)], sampleUV, shadowTexelSize);
			}
			else
			{
				vec2 s0 = textureLod(terrainShadowMaps[nonuniformEXT(i)], sampleUV.xz, 0.0).rg;
				float d0 = clamp((uv.y - s0.g) * 5000.0, 0.0, 10.0) * 0.1;
				float v0 = mix(s0.r, 1.0, d0);
				value = 1.0 - v0;
				//value = 1.0 - (sampleUV.y <= s0.g ? s0.r : 1.0);
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
		vec2 s0 = textureLod(terrainShadowMaps[nonuniformEXT(i)], sampleUV.xz, 0.0).rg;
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

/*vec4 TerrainOcclusion(vec3 worldPosition, vec3 worldDirection)
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
}*/

float TerrainOcclusionCascade(vec2 worldPosition, int cascade)
{
	float result = 0.5;
	int i = clamp(cascade, 0, 2);

	if (variables.glillOffsets[i].y == 1) {return (result);}

	vec2 uv = worldPosition * 0.0001 - (variables.glillOffsets[i].xz - variables.terrainOffset.xz);
	uv = (uv * 10000.0) / variables.glillOffsets[i].w;

	if (max(abs(uv.x), abs(uv.y)) <= 0.5)
	{
		float occlusion = textureLod(glillMaps[nonuniformEXT(i)], uv + 0.5, 0.0).r;

		//occlusion = ((exp(pow(occlusion * 2.0 - 2.0, 3.0))) + (pow(occlusion, 2.0))) * 0.5;
		occlusion = Exaggerate(occlusion);
		occlusion = mix(0.1, 1.0, occlusion);
			
		//return (occlusion);
		result = occlusion;
	}

	return (result);
}

float TerrainOcclusion(vec2 worldPosition)
{
	float result = 0.5;

	//startingCascade = clamp(startingCascade, 0, 2);

	for (int i = 0; i < 3; i++)
	{
		if (variables.glillOffsets[i].y == 1) {return (result);}

		//vec2 uv = worldPosition * 0.0001 - (variables.glillOffsets[i].xz - variables.terrainOffset.xz);
		//uv = (uv * 10000.0) / variables.glillOffsets[i].w;

		vec2 uv = (worldPosition - (10000.0 * (variables.glillOffsets[i].xz - variables.terrainOffset.xz))) / variables.glillOffsets[i].w;

		if (max(abs(uv.x), abs(uv.y)) <= 0.5)
		{
			float occlusion = textureLod(glillMaps[nonuniformEXT(i)], uv + 0.5, 0.0).r;

			//occlusion = ((exp(pow(occlusion * 2.0 - 2.0, 3.0))) + (pow(occlusion, 2.0))) * 0.5;
			occlusion = Exaggerate(occlusion);
			occlusion = mix(0.1, 1.0, occlusion);
			
			//return (occlusion);
			result = occlusion;
			break;
		}
	}

	return (result);
}

//float TerrainOcclusion(vec2 worldPosition)
//{
//	return (TerrainOcclusion(worldPosition, 0));
//}

void SetTerrainIllumination()
{
	vec3 worldDirection = vec3(0.0, 1.0, 0.0);
	vec3 skyWorldPosition = ((vec3(0.0, 0.0, 0.0) + vec3(0.0, maxHeight * 0.5, 0.0)) * atmosphereData.cameraScale) + vec3(0.0, bottomRadius, 0.0);
	//vec3 skyWorldPosition = ((variables.viewPosition.xyz + vec3(0.0, maxHeight * 0.5 + (variables.terrainOffset.y * 10000.0), 0.0)) * atmosphereData.cameraScale) + vec3(0.0, bottomRadius, 0.0);
	vec3 up = normalize(skyWorldPosition);
	float viewAngle = acos(dot(worldDirection, up));
	float viewHeight = length(skyWorldPosition);

	//float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
	float lightAngle = acos(dot(variables.lightDirection.xyz, worldDirection));
	bool groundIntersect = IntersectSphere(skyWorldPosition, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

	vec2 skyUV = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

	//float disInter = 1.0 - pow(1.0 - clamp(SquaredDistance(worldPosition, variables.viewPosition.xyz) / (variables.resolution.w * variables.resolution.w), 0.0, 1.0), atmosphereData.skyDilute);

	//vec3 skyColor = texture(skyMap, skyUV).rgb * atmosphereData.defaultSkyPower * mix(atmosphereData.skyPower, 1.0, disInter);
	vec3 skyColor = texture(skyMap, skyUV).rgb;

	atmosphereData.skyColor = vec4(skyColor, 0.0);

	//return (skyColor);
}

vec3 TerrainIllumination(vec3 worldPosition, vec3 worldDirection)
{
	float disInter = 1.0 - pow(1.0 - clamp(SquaredDistance(worldPosition, variables.viewPosition.xyz) / (variables.resolution.w * variables.resolution.w), 0.0, 1.0), atmosphereData.skyDilute);

	return (atmosphereData.skyColor.rgb * atmosphereData.defaultSkyPower * mix(atmosphereData.skyPower, 1.0, disInter));

	/*if (variables.terrainOffset.w > 0) {worldDirection = vec3(0.0, 1.0, 0.0);}
	vec3 skyWorldPosition = ((worldPosition + vec3(0.0, maxHeight * 0.5 + (variables.terrainOffset.y * 10000.0), 0.0)) * atmosphereData.cameraScale) + vec3(0.0, bottomRadius, 0.0);
	//vec3 skyWorldPosition = ((variables.viewPosition.xyz + vec3(0.0, maxHeight * 0.5 + (variables.terrainOffset.y * 10000.0), 0.0)) * atmosphereData.cameraScale) + vec3(0.0, bottomRadius, 0.0);
	vec3 up = normalize(skyWorldPosition);
	float viewAngle = acos(dot(worldDirection, up));
	float viewHeight = length(skyWorldPosition);

	//float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
	float lightAngle = acos(dot(variables.lightDirection.xyz, worldDirection));
	bool groundIntersect = IntersectSphere(skyWorldPosition, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

	vec2 skyUV = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

	float disInter = 1.0 - pow(1.0 - clamp(SquaredDistance(worldPosition, variables.viewPosition.xyz) / (variables.resolution.w * variables.resolution.w), 0.0, 1.0), atmosphereData.skyDilute);

	vec3 skyColor = texture(skyMap, skyUV).rgb * atmosphereData.defaultSkyPower * mix(atmosphereData.skyPower, 1.0, disInter);

	return (skyColor);*/
}

const float dif9 = 1.0 / 9.0;
const float dif3 = 1.0 / 3.0;
const float dif12 = 1.0 / 12.0;

float BlendShadows(int i, vec2 uv, float refDepth, int mode)
{
	if (mode == 0)
	{
		return (texture(shadowMaps[nonuniformEXT(i)], vec3(uv, refDepth)));
	}
	else if (mode == 1)
	{
		vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps[nonuniformEXT(i)], 0).xy);
		float result = 0.0;

		for (int x = 0; x <= 1; x++)
		{
			for (int y = 0; y <= 1; y++)
			{
				vec2 coords = uv + (vec2(x, y) - 0.5) * texelSize;
				if (abs(coords.x - 0.5) > 0.5 || abs(coords.y - 0.5) > 0.5) continue;
				result += texture(shadowMaps[nonuniformEXT(i)], vec3(coords.xy, refDepth));
			}
		}
		return (result * 0.25);
	}
	else if (mode == 2)
	{
		vec2 texelSize = 1.0 / vec2(textureSize(shadowMaps[nonuniformEXT(i)], 0).xy);
		float result = 0.0;

		for (int x = -1; x <= 1; x++)
		{
			for (int y = -1; y <= 1; y++)
			{
				vec2 coords = uv + vec2(x, y) * texelSize;
				if (abs(coords.x - 0.5) > 0.5 || abs(coords.y - 0.5) > 0.5) continue;
				result += texture(shadowMaps[nonuniformEXT(i)], vec3(coords.xy, refDepth));
			}
		}
		return (result * dif9);
		//return (1.0 - clamp(result, 0.0, 1.0));
	}

	return (1.0);
}

float SampleShadows(vec3 worldPosition)
{
	if (shadowData.enabled == 0) {return (1.0);}

	float result = 0.0;
	float blend = 0.0;

	for (int i = 0; i < 3; i++)
	{
		vec4 lightClip = variables.shadowMatrices[i] * vec4(worldPosition, 1.0);
		vec3 ndc = lightClip.xyz / lightClip.w;
		vec2 uv = ndc.xy * 0.5 + 0.5;
		//float refDepth = ndc.z - bias;
		float refDepth = ndc.z;

		if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || ndc.z < 0.0 || ndc.z > 1.0) {continue;}

		if (i == 0 && SquaredDistance(worldPosition, variables.viewPosition.xyz) > (100 * 100)) {continue;}
		if (i == 1 && SquaredDistance(worldPosition, variables.viewPosition.xyz) > (500 * 500)) {continue;}

		float fade = 0.0;
		if (i== 0 || i == 2)
		{
			float border = 0.0;
			border = max(border, abs(uv.x - 0.5));
			border = max(border, abs(uv.y - 0.5));
			border = max(border, abs(ndc.z - 0.5));
			if (border > 0.4) {fade = clamp((border - 0.4) * 10.0, 0.0, 1.0);}
		}

		float inter = clamp(SquaredDistance(worldPosition, variables.viewPosition.xyz) / (1000.0 * 1000.0), 0.0, 1.0);

		int mode = 0;
		if (inter < shadowData.blend1Dis) {mode = 1;}
		if (inter < shadowData.blend0Dis) {mode = 2;}

		if (fade >= 1.0) {return (1.0);}

		if (i == 1 && blend > 0.0)
		{
			fade = 1.0 - blend;
		}

		result += (1.0 - BlendShadows(i, uv, refDepth, mode)) * (1.0 - fade);

		if (i == 0 && fade > 0.0)
		{
			blend = fade;
			continue;
		}
		
		//return (result);

		break;

		//result += (1.0 - BlendShadows(i, uv, refDepth, mode)) * (1.0 - fade) * (blend);
		//if (fade == 0.0) {break;}
		//blend = fade;
	}

	return (1.0 - clamp(result, 0.0, 1.0));
	//return (1.0 - clamp(result, 0.0, 1.0));
}

#endif