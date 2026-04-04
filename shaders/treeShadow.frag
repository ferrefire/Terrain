#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

/*struct TreeShaderConfig
{
	int weightPower;
	float uvScale;
	float glillNormalMix;
	float normalStrength;
	int textureLod;
	float ambientStrength;
};

layout(set = 1, binding = 1) uniform sampler2D barkTextures[3];

layout(set = 1, binding = 2, std140) uniform treeShaderConfig
{
	TreeShaderConfig config;
};

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 terrainValues;
layout(location = 3) flat in int lod;*/

layout(location = 0) out vec4 pixelColor;

void main()
{
	pixelColor = vec4(1.0);
}