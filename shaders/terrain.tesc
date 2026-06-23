#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

layout(location = 0) flat in int lod[];
layout(location = 1) in float lodInter[];
layout(location = 2) in vec4 vertTerrainValues[];
layout(location = 3) flat in int id[];

layout(vertices = 3) out;
layout(location = 0) patch out int patchLod;
layout(location = 1) patch out int patchId;

#include "noise.glsl"
#include "sampling.glsl"
#include "transformation.glsl"
#include "terrainFunctions.glsl"
#include "culling.glsl"

const float tesselationFactor = 15;

float TessellationFactor (vec3 p0, vec3 p1)
{
    float edgeLength = distance(p0, p1);
    vec3 edgeCenter = (p0 + p1) * 0.5;
    //float viewDistance = distance(edgeCenter, variables.viewPosition.xyz);
    float viewDistance = max(0.0, distance(edgeCenter, variables.viewPosition.xyz) - 25.0);
    //return (edgeLength * variables.resolution.y * (1.0 / (factor * viewDistance)));
    return (edgeLength * 1000.0 * (1.0 / (tesselationFactor * viewDistance)));
}

float TessellationFactorScreen (vec3 p0, vec3 p1, float factor)
{
    float result = distance(p0, p1) * (variables.resolution.y / factor);

	return (max(1, result));
}

float TessellationFactorDepth (vec3 p, float factor)
{
    float result = (1.0 - p.z) * (variables.resolution.y / factor);

	return (max(1, result));
}

bool CullWinding(vec3 pos, vec4 terrainValues)
{
	//vec4 terrainValues = vec4(0);
	float cullAngle = 0.4;

	return (dot(terrainValues.yzw, normalize(pos - variables.viewPosition.xyz)) > cullAngle);
}

void main()
{
	vec3 p0 = (gl_in[0].gl_Position).xyz;
    vec3 p1 = (gl_in[1].gl_Position).xyz;
    vec3 p2 = (gl_in[2].gl_Position).xyz;

	vec3 center = (p0 + p1 + p2) * (1.0 / 3.0);
	
	bool cull = false;
	vec3 centerClip = WorldToClip(center);

	if (centerClip.z > 0.01)
	{
		bool centerInView = true;
		bool cornersInView = InView(p0, vec3(0)) == 1 || InView(p1, vec3(0)) == 1 || InView(p2, vec3(0)) == 1;

		vec4 terrainValues = vec4(-1);

		if (!cornersInView) {centerInView = InView(center, vec3(0)) == 1;}

		if (!centerInView)
		{
			terrainValues = TerrainValues(center.xz);
			float height = terrainValues.x;
			center.y = height * maxHeight;
			centerInView = InView(center, vec3(0)) == 1;
		}

		cull = (!centerInView && !cornersInView);

		//if (!cull)
		if (!cull && lod[0] <= 1)
		{
			if (terrainValues.x == -1) {terrainValues = TerrainValues(center.xz);}
			if (CullWinding(center, terrainValues) && CullWinding(p0, vertTerrainValues[0]) && CullWinding(p1, vertTerrainValues[1]) && CullWinding(p2, vertTerrainValues[2])) {cull = true;}
		}
	}

	//if (!cull)
	//{
	//	if (BehindTerrain(center, 5, 2) == 1 && BehindTerrain(p0, 5, 2) == 1 && BehindTerrain(p1, 5, 2) == 1 && BehindTerrain(p2, 5, 2) == 1) {cull = true;}
	//}
	
	if (cull)
	{
		gl_TessLevelOuter[0] = 0;
    	gl_TessLevelOuter[1] = 0;
    	gl_TessLevelOuter[2] = 0;
    	gl_TessLevelInner[0] = 0;
	}
	else
	{
		float tessLevel1 = TessellationFactor(p1, p2);
    	float tessLevel2 = TessellationFactor(p2, p0);
    	float tessLevel3 = TessellationFactor(p0, p1);

    	gl_TessLevelOuter[0] = tessLevel1;
    	gl_TessLevelOuter[1] = tessLevel2;
    	gl_TessLevelOuter[2] = tessLevel3;
    	gl_TessLevelInner[0] = (tessLevel1 + tessLevel2 + tessLevel3) * (1.0 / 3.0);
	}

	if (gl_InvocationID == 0)
	{
		patchLod = lod[0];
		patchId = id[0];
	}

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}