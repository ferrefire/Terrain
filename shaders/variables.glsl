#ifndef VARIABLES_INCLUDED
#define VARIABLES_INCLUDED

const int cascadeCount = 8;
const float maxHeight = 5000.0;

struct TreeComputeConfig
{
	float treeSpacing;
	float treeOffset;
	float maxSteepness;
	uint overdrawCulling;

	//float noiseSeed;
	float noiseScale;
	int noiseOctaves;
	float noiseCutoff;
	float noiseCutoffRandomness;

	uint occlusionCulling;
	int cullIterations;
	uint cullExponent;
	float cullStartDistance;

	int radiuses[4];
	int shadowRadiuses[4];

	int squaredLengths[4];
	int squaredShadowLengths[4];

	float squaredDistances[4];

	int leafCounts[4];

	vec4 treesCenter;

	int overdrawCullingMinimum;
	int overdrawCullingMaximum;
	float overdrawCullingHeightOffset;
	uint overdrawCullingHeightOnly;
	uint overdrawLodCull;
	int overdrawLodCullMinimum;
	int overdrawLodCullMaximum;
	uint overdrawMisses;
	float cullingHeight;
};

struct LeafData
{
	vec4 position;
	vec4 rotationXY;
	vec4 rotationZ;
	//mat4 rotation;
};

struct LeafShaderConfig
{
	float localNormalBlend;
	float worldNormalHeight;
	float translucencyBlend;
	float shadowTranslucencyDim;
	float translucencyBias;
	float translucencyRange;

	float lod0Size;
	float lod1Size;
	float lod2Size;
	float lod3Size;

	uint lodInterMod;
	float lodInterPow;
};

layout(set = 0, binding = 0, std140) uniform Variables
{
	mat4 view;
	mat4 projection;
	mat4 shadowMatrices[3];
	vec4 viewPosition;
	vec4 viewDirection;
	vec4 lightDirection;
	vec4 resolution;
	vec4 terrainOffset;
	vec4 heightmapOffsets[cascadeCount];
	vec4 shadowmapOffsets[3];
	vec4 glillOffsets[3];
} variables;

#endif