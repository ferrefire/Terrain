#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

layout(set = 2, binding = 0) readonly uniform models { mat4 model; } object;

layout(location = 0) in vec3 localPosition;

layout(location = 0) flat out int lod;
layout(location = 1) out float lodInter;
layout(location = 2) out vec4 vertTerrainValues;
layout(location = 3) flat out int id;

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
	vec3 worldPosition = localPosition * terrainSize;
	int instanceIndex = gl_InstanceIndex;
	int xi = terrainRadius;
	int yi = terrainRadius;
	//lod = gl_InstanceIndex;
	lod = 0;
	lodInter = 0.0;

	id = gl_InstanceIndex;

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

	vec4 terrainValues = TerrainValues(worldPosition.xz);
	vertTerrainValues = terrainValues;

	worldPosition.y = terrainValues.x * maxHeight - variables.terrainOffset.y * terrainDiv;

	gl_Position = vec4(worldPosition, 1.0);
	//gl_Position = variables.projection * variables.view * object.model * vec4(sampledPosition, 1.0);
}