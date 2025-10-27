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
	vec4 heightmapOffsets[8];
} variables;

//layout(set = 0, binding = 1) uniform sampler2D heightmaps[3];

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

//layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(location = 0) flat in int lod[];
layout(location = 1) in float lodInter[];
layout(location = 2) in vec4 vertTerrainValues[];

layout(vertices = 3) out;
layout(location = 0) patch out int patchLod;

#include "noise.glsl"
#include "sampling.glsl"
#include "culling.glsl"
#include "transformation.glsl"
#include "terrainFunctions.glsl"

const float tesselationFactor = 10;

float TessellationFactor (vec3 p0, vec3 p1, float factor)
{
    float edgeLength = distance(p0, p1);
    vec3 edgeCenter = (p0 + p1) * 0.5;
    float viewDistance = distance(edgeCenter, variables.viewPosition.xyz);
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
	//if (lod[0] > 0) cullAngle = 0.75;

	//if (lod[0] == 0) {terrainValues = TerrainValuesLod(pos.xz, cascadeCount - 2);}
	//else {terrainValues = TerrainValues(pos.xz);}

	//else if (terrainValues.x == -1) {terrainValues = TerrainValues(center.xz);}

	//terrainValues = TerrainValues(pos.xz);
	//terrainValues = TerrainValuesLod(pos.xz, cascadeCount);

	//if (lod[0] == 1) {cullAngle = 0.75;}
	//if (lod[0] == 2) {cullAngle = 0.75;}

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
		bool centerInView = InView(center, vec3(0)) == 1;
		bool cornersInView = InView(p0, vec3(0)) == 1 || InView(p1, vec3(0)) == 1 || InView(p2, vec3(0)) == 1;

		vec4 terrainValues = vec4(-1);

		if (!centerInView)
		{
			//vec2 uv = (center.xz / 10000.0) + variables.terrainOffset.xz;
			//float height = TerrainData(uv, int(variables.terrainOffset.w) - lod[0], true).x;
			//float height = TerrainData(uv, int(variables.terrainOffset.w) - (lod[0] == 0 ? 0 : 5), true).x;
			terrainValues = TerrainValues(center.xz);
			float height = terrainValues.x;
			center.y = height * 5000.0;
			centerInView = InView(center, vec3(0)) == 1;
		}

		cull = (!centerInView && !cornersInView);

		//if (!cull && lod[0] <= 1)
		if (!cull)
		{
			/*//if (terrainValues.x == -1) {terrainValues = TerrainValues(center.xz);}
			float cullAngle = 0.5;
			//if (lod[0] > 0) cullAngle = 0.75;
			if (lod[0] == 0) {terrainValues = TerrainValuesLod(center.xz, cascadeCount - 2);}
			else if (terrainValues.x == -1) {terrainValues = TerrainValues(center.xz);}

			if (lod[0] == 1) {cullAngle = 0.75;}
			if (lod[0] == 2) {cullAngle = 0.75;}

			if (dot(terrainValues.yzw, normalize(center - variables.viewPosition.xyz)) > cullAngle) {cull = true;}*/

			//if (CullWinding(center) && CullWinding(p0) && CullWinding(p1) && CullWinding(p2)) {cull = true;}
			if (terrainValues.x == -1) {terrainValues = TerrainValues(center.xz);}
			if (CullWinding(center, terrainValues) && CullWinding(p0, vertTerrainValues[0]) && CullWinding(p1, vertTerrainValues[1]) && CullWinding(p2, vertTerrainValues[2])) {cull = true;}
		}
	}
	
	if (cull)
	{
		gl_TessLevelOuter[0] = 0;
    	gl_TessLevelOuter[1] = 0;
    	gl_TessLevelOuter[2] = 0;
    	gl_TessLevelInner[0] = 0;
	}
	else
	{
		float factor = 20;
		//if (lod[0] == 1) factor = 15;
		//if (lod[0] == 0) factor = 10;

		//factor -= 5;

		//if (lod[0] == 2 && lodInter[0])
		//factor = mix(10, 20, lodInter[0]);

		float tessLevel1 = TessellationFactor(p1, p2, factor);
    	float tessLevel2 = TessellationFactor(p2, p0, factor);
    	float tessLevel3 = TessellationFactor(p0, p1, factor);

		//vec3 p0CS = WorldToClip(p0);
		//vec3 p1CS = WorldToClip(p1);
		//vec3 p2CS = WorldToClip(p2);
		//float tessLevel1 = TessellationFactorScreen(p1CS, p2CS, factor);
    	//float tessLevel2 = TessellationFactorScreen(p2CS, p0CS, factor);
    	//float tessLevel3 = TessellationFactorScreen(p0CS, p1CS, factor);
		//float tessLevel1 = TessellationFactorDepth(p0CS, factor);
    	//float tessLevel2 = TessellationFactorDepth(p1CS, factor);
    	//float tessLevel3 = TessellationFactorDepth(p2CS, factor);

    	gl_TessLevelOuter[0] = tessLevel1;
    	gl_TessLevelOuter[1] = tessLevel2;
    	gl_TessLevelOuter[2] = tessLevel3;
    	gl_TessLevelInner[0] = (tessLevel1 + tessLevel2 + tessLevel3) * (1.0 / 3.0);
	}

	/*float tessLevel1 = TessellationFactor(p1, p2);
    float tessLevel2 = TessellationFactor(p2, p0);
    float tessLevel3 = TessellationFactor(p0, p1);

    gl_TessLevelOuter[0] = tessLevel1;
    gl_TessLevelOuter[1] = tessLevel2;
    gl_TessLevelOuter[2] = tessLevel3;
    gl_TessLevelInner[0] = (tessLevel1 + tessLevel2 + tessLevel3) * (1.0 / 3.0);*/

	if (gl_InvocationID == 0) patchLod = lod[0];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}