#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "transformation.glsl"
#include "random.glsl"

layout(push_constant, std430) uniform pc
{
    uint cascade;
};

layout(set = 1, binding = 1, std430) readonly buffer TreeRenderBuffer
{
	TreeData renderData[];
};

layout(set = 1, binding = 2, std430) readonly buffer LeafDataBuffer
{
	LeafData leafData[];
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

const int factors[5] = {0, 3, 3 * 6, 3 * 6 * 9, 3 * 6 * 9 * 3};

void main()
{
	int instanceIndex = gl_InstanceIndex;
	int baseInstance = gl_BaseInstance;

	int treeIndex = instanceIndex / config.leafCounts[0];
	int leafIndex = instanceIndex % config.leafCounts[0];

	float scalar = 1.0;
	int instanceLod = 0;
	const float scales[5] = {shaderConfig.lod0Size, shaderConfig.lod1Size, shaderConfig.lod2Size, shaderConfig.lod3Size, shaderConfig.lod4Size};

	for (int i = 4; i > 0; i--)
	{
		if (baseInstance == config.squaredShadowLengths[i - 1] * config.leafCounts[0])
		{
			treeIndex = (baseInstance / config.leafCounts[0]) + ((instanceIndex - baseInstance) / config.leafCounts[i]);
			float inter = float((instanceIndex - baseInstance) % config.leafCounts[i]) / float(config.leafCounts[i]);
			leafIndex = int(round(mix(0.0, config.leafCounts[0], inter)));
			scalar = scales[i];
			instanceLod = i;
		}
	}

	TreeData currentTree = renderData[treeIndex];
	LeafData currentLeaf = leafData[leafIndex];

	int lod = int(floor(currentTree.position.w));
	float lodInter = currentTree.position.w - float(lod);
	if (shaderConfig.lodInterMod == 1) {lodInter = pow(lodInter, shaderConfig.lodInterPow);}

	float scaleMult = mix(0.8, 1.25, rand01(leafIndex));

	scalar *= scaleMult;

	scalar *= 0.625;
	//scalar *= 1.5;

	vec3 leafPosition = localPosition;

	if (shaderConfig.scaleWithTree == 1)
	{
		float scaleWeight = currentTree.rotation.x;
		leafPosition *= scaleWeight;
	}

	leafPosition *= scalar;
	leafPosition = RotateX(leafPosition, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y);
	leafPosition = RotateY(leafPosition, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w);
	leafPosition = RotateZ(leafPosition, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y);
	//leafPosition = (currentLeaf.rotation * vec4(leafPosition, 1.0)).xyz;
	leafPosition += currentLeaf.position.xyz * currentTree.rotation.x;
	leafPosition = RotateY(leafPosition, currentTree.rotation.z, currentTree.rotation.w);
	leafPosition.y -= 0.5;

	vec3 worldPosition = leafPosition + currentTree.position.xyz;

	gl_Position = variables.shadowMatrices[cascade] * vec4(worldPosition, 1.0);
}