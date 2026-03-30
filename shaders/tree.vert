#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "transformation.glsl"

struct TreeData
{
	vec4 position;
	vec4 terrainValues;
};

layout(set = 1, binding = 0, std430) buffer TreeBuffer
{
	TreeData data[];
};

layout(location = 0) in vec3 localPosition;
layout(location = 1) in vec3 localNormal;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec4 terrainValues;

void main()
{
	int instanceIndex = gl_InstanceIndex;

	//Precalculate sin and cos rotations

	worldNormal = normalize(RotateY(localNormal, data[instanceIndex].position.w));

	vec3 treePosition = RotateY(localPosition, data[instanceIndex].position.w);
	treePosition.y += 0.5;
	treePosition.y *= 20.0;
	treePosition.y -= 0.5;

	worldPosition = treePosition + data[instanceIndex].position.xyz;

	terrainValues = data[instanceIndex].terrainValues;

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}