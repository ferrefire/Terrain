#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "transformation.glsl"
#include "random.glsl"

layout(set = 1, binding = 0, std430) readonly buffer TreeRenderBuffer
{
	TreeData renderData[];
};

layout(set = 1, binding = 2, std430) readonly buffer LeafDataBuffer
{
	LeafData leafData[];
};

layout(set = 1, binding = 3, std430) readonly buffer TreeComputeConfigBuffer
{
	TreeComputeConfig config;
};

layout(set = 1, binding = 4, std140) uniform LeafShaderConfigBuffer
{
	LeafShaderConfig shaderConfig;
};

layout(location = 0) in vec3 localPosition;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 localNormal;
layout(location = 3) out vec4 terrainValues;
layout(location = 4) out float lod;
layout(location = 5) out float variant;
//layout(location = 6) out vec3 localCoords;

//const int leafCount = 2916;
//const int leafCount1 = leafCount / 3;
//const int leafCount2 = leafCount1 / 9;
//const int leafCount3 = leafCount2 / 9;

//const float scales[4] = {1.0, 2.5, 5.0, 12.0};
const int factors[4] = {0, 3, 3 * 6, 3 * 6 * 9};

void main()
{
	int instanceIndex = gl_InstanceIndex;
	int baseInstance = gl_BaseInstance;

	int treeIndex = instanceIndex / config.leafCounts[0];
	int leafIndex = instanceIndex % config.leafCounts[0];

	float scalar = 1.0;
	const float scales[4] = {shaderConfig.lod0Size, shaderConfig.lod1Size, shaderConfig.lod2Size, shaderConfig.lod3Size};

	for (int i = 3; i > 0; i--)
	{
		if (baseInstance == config.squaredLengths[i - 1] * config.leafCounts[0])
		{
			treeIndex = (baseInstance / config.leafCounts[0]) + ((instanceIndex - baseInstance) / config.leafCounts[i]);
			float inter = float((instanceIndex - baseInstance) % config.leafCounts[i]) / float(config.leafCounts[i]);
			leafIndex = int(floor(mix(0.0, config.leafCounts[0], inter)));
			scalar = scales[i];
		}
	}

	TreeData currentTree = renderData[treeIndex];
	LeafData currentLeaf = leafData[leafIndex];

	int iLod = int(floor(currentTree.position.w));
	float lodInter = currentTree.position.w - float(iLod);
	if (shaderConfig.lodInterMod == 1) {lodInter = pow(lodInter, shaderConfig.lodInterPow);}

	//if (lodInter > 0.75) {lodInter = (lodInter - 0.75) * 4.0;}
	//else {lodInter = 0;}

	float scaleMult = mix(0.8, 1.25, rand01(leafIndex));

	if (iLod < 3)
	{
		scalar = mix(scales[iLod], 0.0, lodInter);
		if (leafIndex % factors[iLod + 1] == 0) {scalar = mix(scales[iLod], scales[iLod + 1], lodInter);}
		//if (lod == 0 && leafIndex % factors[lod + 1] == 0) {scalar = scales[lod + 1];}
	}

	lod = currentTree.position.w;

	scalar *= scaleMult;

	scalar *= 0.75;

	//Improve this by rotating with one matrix for all rotations instead of one for each axis!!!

	vec3 leafPosition = localPosition * scalar;
	if (shaderConfig.scaleWithTree == 1) {leafPosition *= clamp(currentTree.rotation.x, 0.0, 1.0);}
	//if (shaderConfig.scaleWithTree == 1) {leafPosition *= mix(currentTree.rotation.x, 1.0, 0.75);}

	leafPosition = RotateX(leafPosition, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y);
	leafPosition = RotateY(leafPosition, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w);
	leafPosition = RotateZ(leafPosition, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y);

	//leafPosition = (currentLeaf.rotation * vec4(leafPosition, 1.0)).xyz;

	leafPosition += currentLeaf.position.xyz * currentTree.rotation.x;
	leafPosition = RotateY(leafPosition, currentTree.rotation.z, currentTree.rotation.w);
	leafPosition.y -= 0.5;

	worldNormal = normalize(leafPosition - vec3(0.0, shaderConfig.worldNormalHeight, 0.0));
	localNormal = vec3(0, 0, 1);
	
	localNormal = normalize(RotateX(localNormal, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y));
	localNormal = normalize(RotateY(localNormal, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w));
	localNormal = normalize(RotateZ(localNormal, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y));

	//localNormal = normalize((currentLeaf.rotation * vec4(localNormal, 0.0)).xyz);

	localNormal = normalize(RotateY(localNormal, currentTree.rotation.z, currentTree.rotation.w));
	//worldNormal = localNormal;

	//localCoords = localPosition;

	//variant = instanceIndex % 3;
	variant = scaleMult;

	worldPosition = leafPosition + currentTree.position.xyz;

	terrainValues = currentTree.terrainValues;

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}