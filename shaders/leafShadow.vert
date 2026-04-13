#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "transformation.glsl"
#include "random.glsl"

struct TreeData
{
	vec4 position;
	vec4 rotation;
	vec4 terrainValues;
};

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

layout(set = 1, binding = 3, std430) readonly buffer TreeComputeConfigBuffer
{
	TreeComputeConfig config;
};

layout(set = 1, binding = 4, std140) uniform LeafShaderConfigBuffer
{
	LeafShaderConfig shaderConfig;
};

layout(location = 0) in vec3 localPosition;

const int factors[4] = {0, 3, 3 * 6, 3 * 6 * 9};

void main()
{
	int instanceIndex = gl_InstanceIndex;
	int baseInstance = gl_BaseInstance;

	int treeIndex = instanceIndex / config.leafCounts[0];
	int leafIndex = instanceIndex % config.leafCounts[0];

	float scalar = 1.0;
	int instanceLod = 0;
	const float scales[4] = {shaderConfig.lod0Size, shaderConfig.lod1Size, shaderConfig.lod2Size, shaderConfig.lod3Size};

	for (int i = 3; i > 0; i--)
	{
		if (baseInstance == config.squaredShadowLengths[i - 1] * config.leafCounts[0])
		{
			treeIndex = (baseInstance / config.leafCounts[0]) + ((instanceIndex - baseInstance) / config.leafCounts[i]);
			float inter = float((instanceIndex - baseInstance) % config.leafCounts[i]) / float(config.leafCounts[i]);
			leafIndex = int(floor(mix(0.0, config.leafCounts[0], inter)));
			scalar = scales[i];
			instanceLod = i;
		}
	}

	TreeData currentTree = renderData[treeIndex];
	LeafData currentLeaf = leafData[leafIndex];

	int lod = int(floor(currentTree.position.w));
	float lodInter = currentTree.position.w - float(lod);
	//lodInter = pow(lodInter, 2);

	//if (lodInter > 0.75) {lodInter = (lodInter - 0.75) * 4.0;}
	//else {lodInter = 0;}

	float scaleMult = mix(0.8, 1.25, rand01(leafIndex));

	if (lod < 3)
	{
		if (cascade == lod - 1)
		{
			scalar = 0;
			if (leafIndex % factors[lod + 1] == 0) {scalar = mix(scales[lod], scales[lod + 1], lodInter);}
			if (leafIndex % factors[lod] == 0) {scalar = mix(scales[lod], 0.0, lodInter);}
		}
		else
		{
			scalar = mix(scales[lod], 0.0, lodInter);
			if (leafIndex % factors[lod + 1] == 0) {scalar = mix(scales[lod], scales[lod + 1], lodInter);}
			//if (lod == 0 && leafIndex % factors[lod + 1] == 0) {scalar = scales[lod + 1];}
		}
	}

	scalar *= scaleMult;

	scalar *= 0.75;

	vec3 leafPosition = localPosition;

	//vec3 localNormal = vec3(0, 0, 1);
	//localNormal = normalize(RotateY(localNormal, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w));
	//localNormal = normalize(RotateX(localNormal, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y));
	//localNormal = normalize(RotateZ(localNormal, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y));
	//localNormal = normalize(RotateY(localNormal, currentTree.rotation.z, currentTree.rotation.w));
	//if (dot(localNormal, variables.lightDirection.xyz) < 0.0) {leafPosition.x *= -1.0;}

	leafPosition *= scalar;
	leafPosition = RotateY(leafPosition, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w);
	leafPosition = RotateX(leafPosition, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y);
	leafPosition = RotateZ(leafPosition, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y);
	leafPosition += currentLeaf.position.xyz;
	leafPosition = RotateY(leafPosition, currentTree.rotation.z, currentTree.rotation.w);
	leafPosition.y -= 0.5;

	vec3 worldPosition = leafPosition + currentTree.position.xyz;

	gl_Position = variables.shadowMatrices[cascade] * vec4(worldPosition, 1.0);
}