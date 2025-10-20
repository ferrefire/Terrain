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
} variables;

//layout(set = 1, binding = 0) uniform sampler2D heightmap;

layout(set = 2, binding = 0) uniform models { mat4 model; } object;

layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(location = 0) flat out int lod;

#include "noise.glsl"
#include "sampling.glsl"

const int terrainRadius = 8;
const int terrainLength = 2 * terrainRadius + 1;
const int terrainCount = terrainLength * terrainLength;
const float terrainSize = 5000.0;

void main()
{
	//vec3 worldPosition = (object.model * vec4(localPosition, 1.0)).xyz;
	vec3 worldPosition = localPosition * terrainSize;
	int instanceIndex = gl_InstanceIndex;
	lod = gl_InstanceIndex;
	if (instanceIndex > 0)
	{
		instanceIndex -= 1;
		if (instanceIndex >= terrainRadius * terrainLength + terrainRadius) instanceIndex += 1;
		int xi = instanceIndex % terrainLength;
		int yi = instanceIndex / terrainLength;
		worldPosition += vec3(xi - terrainRadius, 0.0, yi - terrainRadius) * terrainSize;
	}

	//vec2 uv = localPosition.xz + 0.5;
	vec2 uv = (worldPosition.xz / 10000.0) + variables.terrainOffset.xz;
	//float height = texture(heightmap, uv).r;

	//vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4, 0.2);
	
	float height = TerrainData(uv, 15, true, false).x;

	//worldNormal = (object.model * vec4(DerivativeToNormal(vec2(hx, hz)), 0.0)).xyz;

	//vec3 sampledPosition = localPosition;
	//sampledPosition.y += height * 0.5;
	worldPosition.y = (height * 0.5) * 10000.0;

	//vec3 worldPosition = (object.model * vec4(sampledPosition, 1.0)).xyz;
	gl_Position = vec4(worldPosition, 1.0);
	//gl_Position = variables.projection * variables.view * object.model * vec4(sampledPosition, 1.0);
}