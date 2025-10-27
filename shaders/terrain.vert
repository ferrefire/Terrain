#version 460

#extension GL_GOOGLE_include_directive : require

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
	vec4 resolution;
	vec4 terrainOffset;
	vec4 heightmapOffsets[8];
} variables;



layout(set = 2, binding = 0) uniform models { mat4 model; } object;

layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(location = 0) flat out int lod;
layout(location = 1) out float lodInter;
layout(location = 2) out vec4 vertTerrainValues;

#include "noise.glsl"
#include "sampling.glsl"
#include "terrainFunctions.glsl"

const int terrainRadius = 2;
const int terrainLength = 2 * terrainRadius + 1;
const int terrainCount = terrainLength * terrainLength;

const int terrainLodRadius = 8;
const int terrainLodLength = 2 * terrainLodRadius + 1;
const int terrainLodCount = terrainLodLength * terrainLodLength;

const float terrainSize = 5000.0;

void main()
{
	//vec3 worldPosition = (object.model * vec4(localPosition, 1.0)).xyz;
	//float scaleMult = 1.0;
	//if (gl_InstanceIndex == 0) {scaleMult = 1.01;}
	//vec3 worldPosition = (localPosition * scaleMult) * terrainSize;
	vec3 worldPosition = localPosition * terrainSize;
	int instanceIndex = gl_InstanceIndex;
	int xi = terrainRadius;
	int yi = terrainRadius;
	//lod = gl_InstanceIndex;
	lod = 0;
	lodInter = 0.0;

	int chunkLod = 0;
	if (gl_InstanceIndex > 0)
	{
		if (gl_InstanceIndex < terrainCount)
		{
			lod = 1;
			instanceIndex -= 1;
			if (instanceIndex >= terrainRadius * terrainLength + terrainRadius) instanceIndex += 1;
			xi = instanceIndex % terrainLength;
			yi = instanceIndex / terrainLength;
			worldPosition += vec3(xi - terrainRadius, 0.0, yi - terrainRadius) * terrainSize;
			worldPosition += vec3(sign(xi - terrainRadius), 0.0, sign(yi - terrainRadius)) * terrainSize * -0.01;
			//worldPosition += vec3(xi - terrainRadius, 0.0, yi - terrainRadius) * terrainSize * -0.01;
			chunkLod = max(abs(xi - terrainRadius), abs(yi - terrainRadius));
		}
		else if (gl_InstanceIndex < terrainLodCount)
		{
			lod = 2;
			instanceIndex -= terrainCount;
			
			for (int i = -terrainRadius; i <= terrainRadius; i++)
			{
				if (instanceIndex < (i + terrainLodRadius) * terrainLodLength + (-terrainRadius + terrainLodRadius)) {break;}

				instanceIndex += terrainLength;
			}

			xi = instanceIndex % terrainLodLength;
			yi = instanceIndex / terrainLodLength;
			worldPosition += vec3(xi - terrainLodRadius, 0.0, yi - terrainLodRadius) * terrainSize;
			worldPosition += vec3(sign(xi - terrainLodRadius), 0.0, sign(yi - terrainLodRadius)) * terrainSize * -0.05;
			//worldPosition += vec3(xi - terrainLodRadius, 0.0, yi - terrainLodRadius) * terrainSize * -0.05;
			chunkLod = max(abs(xi - terrainLodRadius), abs(yi - terrainLodRadius));
		}
	}

	lodInter = float(chunkLod) / float(terrainLodRadius);

	//vec2 uv = localPosition.xz + 0.5;
	//vec2 uv = (worldPosition.xz / 10000.0) + variables.terrainOffset.xz;
	//vec2 uv = (worldPosition.xz / 5000.0) + 0.5;
	//float height = texture(heightmap, uv).r;

	//vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4, 0.2);

	//float lodInter = float(max(abs(xi - terrainRadius), abs(yi - terrainRadius))) / float(terrainRadius);
	//lod = int(round(mix(0, 10, lodInter)));
	
	//float height = TerrainData(uv, int(variables.terrainOffset.w) - lod, true).x;
	//float height = TerrainData(uv, int(variables.terrainOffset.w) - (lod == 0 ? 0 : 5), true).x;
	//float height = texture(heightmaps[1], uv).r;
	//float height = TerrainValues(worldPosition.xz).r;
	vec4 terrainValues = TerrainValues(worldPosition.xz);
	vertTerrainValues = terrainValues;

	//worldNormal = (object.model * vec4(DerivativeToNormal(vec2(hx, hz)), 0.0)).xyz;

	//vec3 sampledPosition = localPosition;
	//sampledPosition.y += height * 0.5;
	worldPosition.y = terrainValues.x * 5000.0;

	//vec3 worldPosition = (object.model * vec4(sampledPosition, 1.0)).xyz;
	gl_Position = vec4(worldPosition, 1.0);
	//gl_Position = variables.projection * variables.view * object.model * vec4(sampledPosition, 1.0);
}