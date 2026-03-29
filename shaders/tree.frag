#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "terrainFunctions.glsl"
#include "lighting.glsl"

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 terrainValues;

layout(location = 0) out vec4 pixelColor;

void main()
{
	float lightStrength = 8;

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * lightStrength;
	data.N = normalize(worldNormal);
	data.albedo = ToLinear(vec3(64.0, 53.0, 40.0) / 255.0);
	data.metallic = 0.0;
	data.roughness = 1.0;

	vec3 diffuse = PBRLighting(data);
	float ao = 1.0;

	float shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * 10000.0, worldPosition.z), 0, true);
	diffuse *= shadow;

	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, data.N, 0.5)));
	vec3 illumination = TerrainIllumination(worldPosition, normalize(terrainValues.yzw));
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(data.N));
	//float occlusion = TerrainOcclusion(worldPosition.xz);
	float occlusion = 1.0;

	vec3 ambientDiffuse = 0.25 * (data.albedo * illumination.rgb);
	vec3 ambient = ambientDiffuse * ao;

	//float aoMult = pow(occlusion, 0.5);

	//diffuse *= ao * aoMult;
	diffuse += ambient * occlusion;

	pixelColor = vec4(diffuse, 1.0);
}