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

layout(set = 1, binding = 3, std430) readonly buffer LeafLodDataBuffer
{
	LeafLodPosition leafLodPositions[];
};


layout(set = 1, binding = 4, std430) readonly buffer TreeComputeConfigBuffer
{
	TreeComputeConfig config;
};

layout(set = 1, binding = 5, std140) readonly uniform LeafShaderConfigBuffer
{
	LeafShaderConfig shaderConfig;
};

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 localNormal;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec3 baseNormal;
layout(location = 3) out vec3 flatNormal;
layout(location = 4) out vec4 terrainValues;
layout(location = 5) out float lod;
layout(location = 6) out float variant;

const int factors[5] = {0, 3, 3 * 6, 3 * 6 * 9, 3 * 6 * 9 * 3};

void main()
{
	int instanceIndex = gl_InstanceIndex;
	int baseInstance = gl_BaseInstance;

	int treeIndex = instanceIndex / config.leafCounts[0];
	int leafIndex = instanceIndex % config.leafCounts[0];

	float scalar = 1.0;
	const float scales[5] = {shaderConfig.lod0Size, shaderConfig.lod1Size, shaderConfig.lod2Size, shaderConfig.lod3Size, shaderConfig.lod4Size};

	for (int i = 4; i > 0; i--)
	{
		if (baseInstance == config.squaredLengths[i - 1] * config.leafCounts[0])
		{
			treeIndex = (baseInstance / config.leafCounts[0]) + ((instanceIndex - baseInstance) / config.leafCounts[i]);
			float inter = float((instanceIndex - baseInstance) % config.leafCounts[i]) / float(config.leafCounts[i]);
			leafIndex = int(round(mix(0, config.leafCounts[0], inter)));
			scalar = scales[i];
		}
	}

	TreeData currentTree = renderData[treeIndex];
	LeafData currentLeaf = leafData[leafIndex];

	int iLod = int(floor(currentTree.position.w));
	float lodInter = currentTree.position.w - float(iLod);
	//float morphInter = 1.0 - pow(1.0 - lodInter, shaderConfig.lodInterPow);
	if (shaderConfig.lodInterMod == 1) {lodInter = pow(lodInter, shaderConfig.lodInterPow);}

	float leafRan = rand01(leafIndex);
	float scaleMult = mix(0.8, 1.25, leafRan);
	//float scaleMult = mix(0.66, 1.5, rand01(leafIndex));

	if (iLod < 4)
	{
		scalar = mix(scales[iLod], 0.0, lodInter);
		if (leafIndex % factors[iLod + 1] == 0) {scalar = mix(scales[iLod], scales[iLod + 1], lodInter);}
		//if (lod == 0 && leafIndex % factors[lod + 1] == 0) {scalar = scales[lod + 1];}
	}

	lod = currentTree.position.w;

	scalar *= scaleMult;

	scalar *= 0.625;
	//scalar *= 1.5;

	//Improve this by rotating with one matrix for all rotations instead of one for each axis!!!

	vec3 leafPosition = localPosition;
	baseNormal = normalize(localNormal);

	if (iLod == 1)
	{
		int vertID = gl_VertexIndex;
		leafPosition = mix(leafPosition, leafLodPositions[vertID].position.xyz, pow(lodInter, 4));
		baseNormal = normalize(mix(baseNormal, leafLodPositions[vertID].normal.xyz, pow(lodInter, 4)));
	}

	leafPosition *= scalar;
	if (shaderConfig.scaleWithTree == 1)
	{
		float scaleWeight = currentTree.rotation.x;
		leafPosition *= scaleWeight;
	}

	leafPosition = RotateX(leafPosition, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y);
	leafPosition = RotateY(leafPosition, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w);
	leafPosition = RotateZ(leafPosition, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y);

	leafPosition += currentLeaf.position.xyz * currentTree.rotation.x;
	leafPosition = RotateY(leafPosition, currentTree.rotation.z, currentTree.rotation.w);
	leafPosition.y -= 0.5;

	worldNormal = normalize(leafPosition - vec3(0.0, shaderConfig.worldNormalHeight * currentTree.rotation.x, 0.0));

	baseNormal = normalize(RotateX(baseNormal, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y));
	baseNormal = normalize(RotateY(baseNormal, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w));
	baseNormal = normalize(RotateZ(baseNormal, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y));
	baseNormal = normalize(RotateY(baseNormal, currentTree.rotation.z, currentTree.rotation.w));

	//variant = 1.0 + rand11(leafIndex + int(ceil(leafRan * 100000))) * 0.25;
	variant = 1.1 + rand11(leafIndex + int(ceil(leafRan * 100000))) * 0.125;

	worldPosition = leafPosition + currentTree.position.xyz;

	terrainValues = currentTree.terrainValues;

	//gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
	gl_Position = variables.projectedView * vec4(worldPosition, 1.0);
}