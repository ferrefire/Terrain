#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

//layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(triangles, fractional_odd_spacing, cw) in;
//layout(triangles, fractional_even_spacing, cw) in;
layout(location = 0) patch in int patchLod;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) flat out int chunkLod;
//layout(location = 1) out vec3 worldNormal;

#include "noise.glsl"
#include "sampling.glsl"
#include "terrainFunctions.glsl"

void main()
{
	vec4 tesselatedPosition = gl_in[0].gl_Position * gl_TessCoord[0] + gl_in[1].gl_Position * gl_TessCoord[1] + gl_in[2].gl_Position * gl_TessCoord[2];
	vec3 sampledPosition = tesselatedPosition.xyz;

	vec4 heightValues = TerrainValues(tesselatedPosition.xz);
	sampledPosition.y = heightValues.x * maxHeight - variables.terrainOffset.y * 10000;

	worldPosition = sampledPosition;


	chunkLod = patchLod;

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}