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

layout(set = 1, binding = 3, std430) readonly buffer LeafLodDataBuffer
{
	vec4 leafLodPositions[];
};

layout(set = 1, binding = 4, std430) readonly buffer TreeComputeConfigBuffer
{
	TreeComputeConfig config;
};

layout(set = 1, binding = 5, std140) uniform LeafShaderConfigBuffer
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

	//if (lodInter > 0.75) {lodInter = (lodInter - 0.75) * 4.0;}
	//else {lodInter = 0;}

	float scaleMult = mix(0.8, 1.25, rand01(leafIndex));
	//float scaleMult = mix(0.66, 1.5, rand01(leafIndex));

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

	scalar *= 0.625;

	vec3 leafPosition = localPosition;
	//if (lod == 1)
	//{
	//	int vertID = gl_VertexIndex;
	//	leafPosition = mix(leafPosition, leafLodPositions[vertID].xyz, pow(lodInter, 2));
	//}

	if (shaderConfig.scaleWithTree == 1)
	{
		float scaleWeight = currentTree.rotation.x;
		//if (scaleWeight > 1.0) {scaleWeight = 1.0 + pow(scaleWeight - 1.0, 6);}
		//if (scaleWeight > 1.0) {scaleWeight = pow(scaleWeight, 0.5);}
		leafPosition *= scaleWeight;
	}
	//if (shaderConfig.scaleWithTree == 1) {leafPosition *= clamp(currentTree.rotation.x, 0.0, 1.0);}
	//if (shaderConfig.scaleWithTree == 1) {leafPosition *= mix(currentTree.rotation.x, 1.0, 0.75);}

	//vec3 localNormal = vec3(0, 0, 1);
	//localNormal = normalize(RotateY(localNormal, currentLeaf.rotationXY.z, currentLeaf.rotationXY.w));
	//localNormal = normalize(RotateX(localNormal, currentLeaf.rotationXY.x, currentLeaf.rotationXY.y));
	//localNormal = normalize(RotateZ(localNormal, currentLeaf.rotationZ.x, currentLeaf.rotationZ.y));
	//localNormal = normalize(RotateY(localNormal, currentTree.rotation.z, currentTree.rotation.w));
	//if (dot(localNormal, variables.lightDirection.xyz) < 0.0) {leafPosition.x *= -1.0;}

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