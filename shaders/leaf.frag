#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "terrainFunctions.glsl"
#include "lighting.glsl"
//#include "functions.glsl"

layout(set = 1, binding = 5, std140) readonly uniform LeafShaderConfigBuffer
{
	LeafShaderConfig shaderConfig;
};

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec3 localNormal;
layout(location = 3) in vec3 flatNormal;
layout(location = 4) in vec4 terrainValues;
layout(location = 5) in float lod;
layout(location = 6) in float variant;

layout(location = 0) out vec4 pixelColor;

void main()
{
	float lightStrength = 8;

	float ao = 1.0;

	vec3 leafNormal = localNormal;

	vec3 mixedNormal = normalize(mix(worldNormal, leafNormal, shaderConfig.localNormalBlend));

	float leafQualityInter = lod / shaderConfig.qualityNormalBlendLodStart;
	if (shaderConfig.qualityNormalBlendLodPower != 1.0) {leafQualityInter =  pow(leafQualityInter, shaderConfig.qualityNormalBlendLodPower);}
	if (lod < shaderConfig.qualityNormalBlendLodStart) {mixedNormal = normalize(mix(leafNormal, mixedNormal, leafQualityInter));}

	vec3 flippedNormal = leafNormal;
	if (!gl_FrontFacing) {flippedNormal *= -1;}

	float colorMult = variant;

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * lightStrength;
	data.N = mixedNormal;
	//data.albedo = ToLinear((vec3(44, 64, 3) * colorMult) / 255.0);
	data.albedo = ToLinear((vec3(46, 54, 2) * colorMult * shaderConfig.colorMult) / 255.0);

	data.roughness = 1.0;
	if (lod < shaderConfig.qualityNormalBlendLodStart) {data.roughness = mix(shaderConfig.qualitySmoothness, 1.0, leafQualityInter);}
	data.metallic = 0.0;

	vec3 diffuse = PBRLighting(data);

	float shadow = 1.0;

	if (shadow > 0.0) {shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * terrainDiv, worldPosition.z));}
	if (shadow > 0.0) {shadow = min(shadow, SampleShadows(worldPosition));}
	diffuse *= shadow;

	vec3 illumination = TerrainIllumination(worldPosition, normalize(terrainValues.yzw));
	float occlusion = shaderConfig.defaultOcclusion;

	float upDot = dot(mixedNormal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
	//float upDot = dot(flippedNormal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
	float illuminationExposure = mix(0.25 * occlusion, 1.5, upDot);
	//float illuminationExposure = mix(0.75 * occlusion, 1.5, upDot);
	vec3 ambientDiffuse = shaderConfig.ambientMult * (data.albedo * (illumination.rgb * illuminationExposure));
	//vec3 ambientDiffuse = config.ambientStrength * (data.albedo * illumination.rgb);
	vec3 ambient = ambientDiffuse * ao;

	diffuse += ambient;

	if (lod < 3.0)
	{
		vec3 viewDirection = normalize(worldPosition - variables.viewPosition.xyz);
		float normDot = clamp(dot(flippedNormal, -variables.lightDirection.xyz), 0.0, 1.0);
		normDot += (1.0 - normDot) * shaderConfig.translucencyBias;

		float translucency = pow(clamp(dot(viewDirection, variables.lightDirection.xyz), 0.0, 1.0), exp2(10 * shaderConfig.translucencyRange + 1)) * 1.0 * normDot;
		translucency *= 1.0 - (1.0 - shadow) * shaderConfig.shadowTranslucencyDim;

		if (lod > 2.0) {translucency *= 1.0 - (lod - 2.0);}

		diffuse += (vec3(1.0, 0.9, 0.7)) * data.albedo * translucency * 16.0;
	}

	pixelColor = vec4(diffuse, 1.0);

	if (shaderConfig.debugMode == 1) {pixelColor = vec4(RandomColor(int(floor(lod))), 1.0);}
	if (shaderConfig.debugMode == 2) {pixelColor = vec4(localNormal * 0.5 + 0.5, 1.0);}
	if (shaderConfig.debugMode == 3) {pixelColor = vec4(data.N * 0.5 + 0.5, 1.0);}
}