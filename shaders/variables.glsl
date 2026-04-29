#ifndef VARIABLES_INCLUDED
#define VARIABLES_INCLUDED

const int cascadeCount = 8;
const float maxHeight = 5000.0;

const int treeBase = 512;
const int shadowCascades = 4;

struct TextureData
{
	vec3 color;
	vec3 normal;
	vec3 arm;
	vec3 weights;
	vec3 uv;
	vec3 baseNormal;
};

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

	int radiuses[8];
	int shadowRadiuses[8];
	int squaredLengths[8];
	int squaredShadowLengths[8];
	float squaredDistances[8];
	int leafCounts[8];
	float cascadeTolerances[4];
	//int radiuses[4];
	//int shadowRadiuses[4];
	//int squaredLengths[4];
	//int squaredShadowLengths[4];
	//float squaredDistances[4];
	//int leafCounts[4];

	vec4 treesCenter;

	int overdrawCullingMinimum;
	int overdrawCullingMaximum;
	float overdrawCullingHeightBase;
	float overdrawCullingHeightOffset;
	uint overdrawCullingHeightOnly;
	uint overdrawLodCull;
	int overdrawLodCullMinimum;
	int overdrawLodCullMaximum;
	uint overdrawMisses;
	uint overdrawLodCullHeavy;

	float cullingHeight;

	float minTreeScale;
	float maxTreeScale;
	float treeScalePower;

	int originalFirstIndex;
	int originalIndexCount;
	uint highQualityLodLeaves;
	uint treeOverdrawForceGround;

	float cascadeCullBias;
};

struct DrawCommand
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    int vertexOffset;
    uint firstInstance;
};

struct TreeData
{
	vec4 position;
	vec4 rotation;
	vec4 terrainValues;
	vec4 info;
};

struct LeafData
{
	vec4 position;
	vec4 rotationXY;
	vec4 rotationZ;
	//mat4 rotation;
};

struct TreeShaderConfig
{
	int weightPower;
	float uvScale;
	float glillNormalMix;
	float normalStrength;
	int textureLod;
	float ambientStrength;
	uint sampleOcclusion;
	float defaultOcclusion;
	uint debugMode;
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
	float lod4Size;

	uint lodInterMod;
	float lodInterPow;

	uint scaleWithTree;
	uint sampleOcclusion;
	uint flipLocalNormal;
	float defaultOcclusion;

	uint debugMode;
	float flatLocalNormalBlend;
	float qualityNormalBlendLodStart;
	float qualityNormalBlendLodPower;
};

struct LeafLodPosition
{
	vec4 position;
	vec4 normal;
};

struct TerrainShaderData
{
	uint debugMode;
	float rockSteepness;
	float rockTransition;
	float snowHeight;
	float snowBlend;
	float snowSteepness;
	uint snowEnabled;
};

layout(set = 0, binding = 0, std140) readonly uniform Variables
{
	mat4 view;
	mat4 projection;
	mat4 shadowMatrices[shadowCascades];
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