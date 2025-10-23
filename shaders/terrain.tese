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

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

//layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(triangles, fractional_odd_spacing, cw) in;
//layout(triangles, fractional_even_spacing, cw) in;
layout(location = 0) patch in int patchLod;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;

#include "noise.glsl"
#include "sampling.glsl"
#include "terrainFunctions.glsl"

void main()
{
	vec4 tesselatedPosition = gl_in[0].gl_Position * gl_TessCoord[0] + gl_in[1].gl_Position * gl_TessCoord[1] + gl_in[2].gl_Position * gl_TessCoord[2];
	//vec2 uv = (tesselatedPosition.xz / 10000.0) + variables.terrainOffset.xz;
	vec2 uv = (tesselatedPosition.xz / 5000.0) + 0.5;
	//vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4, 0.2);
	
	//float viewDistance = distance(variables.viewPosition.xyz, tesselatedPosition.xyz);
	////float dis = 1.0 - viewDistance / variables.resolution.w;
	//int count = 5;
	//if (viewDistance < 5000) count = 15;
	//else if (viewDistance < 6000) count = 14;
	//else if (viewDistance < 7000) count = 13;
	//else if (viewDistance < 8000) count = 12;
	//else if (viewDistance < 9000) count = 11;
	//else if (viewDistance < 10000) count = 10;
	//else if (viewDistance < 11000) count = 9;
	//else if (viewDistance < 12000) count = 8;
	//else if (viewDistance < 13000) count = 7;
	//else if (viewDistance < 14000) count = 6;
	////else if (viewDistance < 15000) count = 10;

	//float height = TerrainData(uv, int(round((1.0 - (viewDistance / variables.resolution.w)) * 10 + 5)), true).x;
	//float height = TerrainData(uv, 5 + int(round(dis * 10.0)), true).x;
	//float height = TerrainData(uv, 15, true, false).x;

	//float viewDistance = distance(variables.viewPosition.xyz, tesselatedPosition.xyz);
	//float qualityInter = clamp(viewDistance, 0.0, 2500.0) / 2500.0;
	//int qualityDescrease = int(round(mix(0, 5, qualityInter)));
	//float height = TerrainData(uv, int(variables.terrainOffset.w) - qualityDescrease, true).x;
	//float height = TerrainData(uv, int(variables.terrainOffset.w) - patchLod, true).x;
	//float height = TerrainData(uv, int(variables.terrainOffset.w) - (patchLod == 0 ? 0 : 5), true).x;
	//float height = texture(heightmaps[1], uv).r;
	float height = TerrainValues(tesselatedPosition.xz).r;

	//worldNormal = DerivativeToNormal(vec2(hx, hz));
	worldNormal = vec3(0);

	//float viewDistance = distance(variables.viewPosition.xyz, tesselatedPosition.xyz);
	//float qualityInter = clamp(viewDistance, 0.0, 2500.0) / 2500.0;
	//int qualityDescrease = int(round(mix(0, 5, qualityInter)));
	//vec3 tnoise = TerrainData(uv, int(variables.terrainOffset.w) - qualityDescrease, false);
	//worldNormal = DerivativeToNormal(vec2(tnoise.y, tnoise.z));
	//float height = tnoise.x;

	vec3 sampledPosition = tesselatedPosition.xyz;
	sampledPosition.y = (height * 0.5) * 10000.0;

	worldPosition = sampledPosition;

	if (patchLod != 0) worldPosition.y -= 5.0;

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}