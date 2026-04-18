#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "transformation.glsl"

layout(set = 1, binding = 0, std430) readonly buffer TreeRenderBuffer
{
	TreeData renderData[];
};

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 localNormal;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec4 terrainValues;
layout(location = 3) out int lod;

void main()
{
	int instanceIndex = gl_InstanceIndex;

	//Precalculate sin and cos rotations

	TreeData currentTree = renderData[instanceIndex];

	worldNormal = normalize(RotateY(localNormal, currentTree.rotation.z, currentTree.rotation.w));

	vec3 treePosition = RotateY(localPosition * currentTree.rotation.x, currentTree.rotation.z, currentTree.rotation.w);
	//treePosition.y += 0.5;
	//treePosition.y *= 20.0;
	treePosition.y -= 0.5;

	worldPosition = treePosition + currentTree.position.xyz;

	terrainValues = currentTree.terrainValues;

	lod = int(floor(currentTree.position.w));

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}