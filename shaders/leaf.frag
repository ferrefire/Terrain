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
//layout(location = 6) in vec3 localCoords;

layout(location = 0) out vec4 pixelColor;

/*const vec4 discardDistances = vec4(0.125 * 1.1, 0.1875, 0.375 * 0.9, 0.0625 * 1.75);

const Triangle discardTriangle0 = Triangle(vec2(0.0, 0.25) * 1.5, vec2(0.25, -0.125) * 1.5, vec2(-0.25, -0.125) * 1.5);
const Triangle discardTriangle1 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(-discardDistances.x, discardDistances.y) * 1.425, vec2(-discardDistances.z, discardDistances.w) * 1.425);
const Triangle discardTriangle2 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(-discardDistances.x, -discardDistances.y) * 1.425, vec2(-discardDistances.z, -discardDistances.w) * 1.425);
const Triangle discardTriangle3 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(discardDistances.x, discardDistances.y) * 1.425, vec2(discardDistances.z, discardDistances.w) * 1.425);
const Triangle discardTriangle4 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(discardDistances.x, -discardDistances.y) * 1.425, vec2(discardDistances.z, -discardDistances.w) * 1.425);
const Triangle discardTriangle5 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(0.25, 0.25) * 1.425, vec2(-0.25, 0.25) * 1.425);

bool ShouldDiscard(vec2 position)
{
	if (InsideTriangle(discardTriangle1, position)) return (true);
	else if (InsideTriangle(discardTriangle2, position)) return (true);
	else if (InsideTriangle(discardTriangle3, position)) return (true);
	else if (InsideTriangle(discardTriangle4, position)) return (true);
	else return (false);
}*/

void main()
{
	//if (!ShouldDiscard(localCoords.xy)) {discard;}

	float lightStrength = 8;

	float ao = 1.0;

	//vec3 _worldNormal = worldNormal;
	//vec3 _localNormal = localNormal;
	//if (!gl_FrontFacing) {_localNormal *= -1;}

	//vec3 leafNormal = flatNormal;
	vec3 leafNormal = localNormal;
	//vec3 leafNormal = normalize(mix(localNormal, flatNormal, shaderConfig.flatLocalNormalBlend));

	//if (shaderConfig.flipLocalNormal == 1 && !gl_FrontFacing) {leafNormal *= -1;}

	//if (dot(leafNormal, worldNormal) < 0.0) {leafNormal *= -1;}

	//vec3 mixedNormal = normalize(mix(_worldNormal, _localNormal, 0.5));
	vec3 mixedNormal = normalize(mix(worldNormal, leafNormal, shaderConfig.localNormalBlend));

	//if (lod < shaderConfig.qualityNormalBlendLodStart) {mixedNormal = normalize(mix(leafNormal, mixedNormal, pow(lod / shaderConfig.qualityNormalBlendLodStart, shaderConfig.qualityNormalBlendLodPower)));}
	float leafQualityInter = lod / shaderConfig.qualityNormalBlendLodStart;
	if (shaderConfig.qualityNormalBlendLodPower != 1.0) {leafQualityInter =  pow(leafQualityInter, shaderConfig.qualityNormalBlendLodPower);}
	if (lod < shaderConfig.qualityNormalBlendLodStart) {mixedNormal = normalize(mix(leafNormal, mixedNormal, leafQualityInter));}

	vec3 flippedNormal = leafNormal;
	if (!gl_FrontFacing) {flippedNormal *= -1;}

	//vec3 flippedNormal = mixedNormal;
	//if (gl_FrontFacing) {flippedNormal *= -1;}

	float colorMult = variant;
	//if (variant == 1) {colorMult = 1.1;}
	//else if (variant == 2) {colorMult = 0.9;}

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * lightStrength;
	data.N = mixedNormal;
	//data.albedo = ToLinear((vec3(44, 64, 3) * colorMult) / 255.0);
	data.albedo = ToLinear((vec3(46, 54, 2) * colorMult * shaderConfig.colorMult) / 255.0);

	//if (lod == 0) {data.albedo = vec3(0, 1, 0);}
	//else if (lod == 1) {data.albedo = vec3(0, 0, 1);}
	//else if (lod == 2) {data.albedo = vec3(1, 0, 0);}

	data.roughness = 1.0;
	if (lod < shaderConfig.qualityNormalBlendLodStart) {data.roughness = mix(shaderConfig.qualitySmoothness, 1.0, leafQualityInter);}
	data.metallic = 0.0;

	vec3 diffuse = PBRLighting(data);

	//vec3 shadowNormal = _worldNormal;
	//if (!gl_FrontFacing) {shadowNormal *= -1;}
	float shadow = 1.0;
	//if (dot(mixedNormal, data.L) < 0.0) {shadow = 0.0;}

	if (shadow > 0.0) {shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * 10000.0, worldPosition.z));}
	if (shadow > 0.0) {shadow = min(shadow, SampleShadows(worldPosition));}
	diffuse *= shadow;

	//vec3 illumination = TerrainIllumination(worldPosition, terrainValues.yzw);
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, _worldNormal, 0.45)));
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, _localNormal, 0.45)));
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, mixedNormal, 0.45)));
	vec3 illumination = TerrainIllumination(worldPosition, normalize(terrainValues.yzw));
	float occlusion = shaderConfig.defaultOcclusion;
	//if (shaderConfig.sampleOcclusion == 1) {occlusion = TerrainOcclusion(worldPosition.xz);}
	//float occlusion = TerrainOcclusion(worldPosition.xz);

	float upDot = dot(mixedNormal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
	//float upDot = dot(flippedNormal, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
	float illuminationExposure = mix(0.25 * occlusion, 1.5, upDot);
	//float illuminationExposure = mix(0.75 * occlusion, 1.5, upDot);
	vec3 ambientDiffuse = 0.25 * (data.albedo * (illumination.rgb * illuminationExposure));
	//vec3 ambientDiffuse = config.ambientStrength * (data.albedo * illumination.rgb);
	vec3 ambient = ambientDiffuse * ao;

	//float aoMult = pow(occlusion, 0.5);

	//diffuse *= ao * aoMult;
	//diffuse += ambient * occlusion;
	diffuse += ambient;

	if (lod < 3.0)
	{
		vec3 viewDirection = normalize(worldPosition - variables.viewPosition.xyz);
		//float normDot = clamp(dot(mixedNormal, variables.lightDirection.xyz), 0.0, 1.0);
		//float normDot = clamp(dot(leafNormal, -variables.lightDirection.xyz), 0.0, 1.0);
		float normDot = clamp(dot(flippedNormal, -variables.lightDirection.xyz), 0.0, 1.0);
		normDot += (1.0 - normDot) * shaderConfig.translucencyBias;

		float translucency = pow(clamp(dot(viewDirection, variables.lightDirection.xyz), 0.0, 1.0), exp2(10 * shaderConfig.translucencyRange + 1)) * 1.0 * normDot;
		translucency *= 1.0 - (1.0 - shadow) * shaderConfig.shadowTranslucencyDim;

		//if (dis > 250.0)
		//{
		//	translucency *= 1.0 - ((dis - 250.0) / 250.0);
		//}

		if (lod > 2.0) {translucency *= 1.0 - (lod - 2.0);}

		diffuse += (vec3(1.0, 0.9, 0.7)) * data.albedo * translucency * 16.0;
	}

	pixelColor = vec4(diffuse, 1.0);

	if (shaderConfig.debugMode == 1) {pixelColor = vec4(RandomColor(int(floor(lod))), 1.0);}
	if (shaderConfig.debugMode == 2) {pixelColor = vec4(localNormal * 0.5 + 0.5, 1.0);}
	if (shaderConfig.debugMode == 3) {pixelColor = vec4(data.N * 0.5 + 0.5, 1.0);}
}