#ifndef VARIABLES_INCLUDED
#define VARIABLES_INCLUDED

const int cascadeCount = 8;

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
	vec4 resolution;
	vec4 terrainOffset;
	vec4 heightmapOffsets[cascadeCount];
	vec4 shadowmapOffsets[3];
} variables;

#endif