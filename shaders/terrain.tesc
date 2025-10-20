#version 460

#extension GL_GOOGLE_include_directive : require

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
	vec4 resolution;
	vec4 terrainOffset;
} variables;

//layout(set = 1, binding = 0) uniform sampler2D heightmap;

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

//layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(location = 0) flat in  int lod[];

layout(vertices = 3) out;
layout(location = 0) patch out int patchLod;

#include "noise.glsl"
#include "sampling.glsl"
#include "culling.glsl"

const float tesselationFactor = 15;

float TessellationFactor (vec3 p0, vec3 p1)
{
    float edgeLength = distance(p0, p1);
    vec3 edgeCenter = (p0 + p1) * 0.5;
    float viewDistance = distance(edgeCenter, variables.viewPosition.xyz);
    return (edgeLength * variables.resolution.y * (1.0 / (tesselationFactor * viewDistance)));
}

void main()
{
	vec3 p0 = (gl_in[0].gl_Position).xyz;
    vec3 p1 = (gl_in[1].gl_Position).xyz;
    vec3 p2 = (gl_in[2].gl_Position).xyz;

	//vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
	//bool cull = (InView(center, vec3(0)) == 0 && InView(p0, vec3(0)) == 0 && InView(p1, vec3(0)) == 0 && InView(p2, vec3(0)) == 0);
	//if (cull)
	//{
	//	gl_TessLevelOuter[0] = 0;
    //	gl_TessLevelOuter[1] = 0;
    //	gl_TessLevelOuter[2] = 0;
    //	gl_TessLevelInner[0] = 0;
	//}
	//else
	//{
	//	float tessLevel1 = TessellationFactor(p1, p2);
    //	float tessLevel2 = TessellationFactor(p2, p0);
    //	float tessLevel3 = TessellationFactor(p0, p1);
    //	gl_TessLevelOuter[0] = tessLevel1;
    //	gl_TessLevelOuter[1] = tessLevel2;
    //	gl_TessLevelOuter[2] = tessLevel3;
    //	gl_TessLevelInner[0] = (tessLevel1 + tessLevel2 + tessLevel3) * (1.0 / 3.0);
	//}

	float tessLevel1 = TessellationFactor(p1, p2);
    float tessLevel2 = TessellationFactor(p2, p0);
    float tessLevel3 = TessellationFactor(p0, p1);

    gl_TessLevelOuter[0] = tessLevel1;
    gl_TessLevelOuter[1] = tessLevel2;
    gl_TessLevelOuter[2] = tessLevel3;
    gl_TessLevelInner[0] = (tessLevel1 + tessLevel2 + tessLevel3) * (1.0 / 3.0);

	if (gl_InvocationID == 0) patchLod = lod[0];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}