#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
#include "terrainFunctions.glsl"
#include "lighting.glsl"

layout(set = 1, binding = 2) uniform sampler2D barkTextures[3];

layout(set = 1, binding = 3, std140) readonly uniform treeShaderConfig
{
	TreeShaderConfig config;
};

layout(location = 0) in vec3 worldPosition;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec4 terrainValues;
layout(location = 3) flat in int lod;

layout(location = 0) out vec4 pixelColor;

void main()
{
	float lightStrength = 8;

	float ao = 1.0;

	vec3 barkDiffuse = ToLinear(vec3(64.0, 53.0, 40.0) / 255.0);
	vec3 barkNormal = worldNormal;
	vec3 barkArm = vec3(1.0, 1.0, 0.0);

	if (lod < config.textureLod)
	{
		vec3 uv = worldPosition * config.uvScale;
		vec3 weights = GetWeights(worldNormal, config.weightPower);

		barkDiffuse = SampleTriplanarColor(barkTextures[0], uv, weights);
		barkNormal = SampleTriplanarNormal(barkTextures[1], uv, weights, worldNormal, config.normalStrength, 0.0);
		barkArm = SampleTriplanarColor(barkTextures[2], uv, weights);

		//roughness = barkArm.g;
		//metallic = barkArm.b;
		ao = barkArm.r;
	}

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * lightStrength;
	data.N = barkNormal;
	//data.albedo = ToLinear(vec3(64.0, 53.0, 40.0) / 255.0);
	data.albedo = barkDiffuse;
	data.roughness = barkArm.g;
	data.metallic = barkArm.b;

	vec3 diffuse = PBRLighting(data);
	//float ao = 1.0;

	//float shadow = 1.0;
	//if (lod > 1) {shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * 10000.0, worldPosition.z), 0, false);}
	//else {shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * 10000.0, worldPosition.z));}
	float shadow = TerrainShadow(vec3(worldPosition.x, worldPosition.y + variables.terrainOffset.y * 10000.0, worldPosition.z));
	if (shadow > 0.0) {shadow = min(shadow, SampleShadows(worldPosition));}
	diffuse *= shadow;

	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, data.N, 0.5)));
	vec3 illumination = TerrainIllumination(worldPosition, normalize(terrainValues.yzw));
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(mix(terrainValues.yzw, worldNormal, config.glillNormalMix)));
	//vec3 illumination = TerrainIllumination(worldPosition, normalize(data.N));
	//float occlusion = TerrainOcclusion(worldPosition.xz);
	float occlusion = config.defaultOcclusion;
	if (config.sampleOcclusion == 1) {occlusion = TerrainOcclusion(worldPosition.xz);}

	//vec3 ambientDiffuse = 0.25 * (data.albedo * illumination.rgb);
	float upDot = dot(data.N, vec3(0.0, 1.0, 0.0)) * 0.5 + 0.5;
	float illuminationExposure = mix(0.25 * occlusion, 1.5, upDot);
	vec3 ambientDiffuse = config.ambientStrength * (data.albedo * (illumination.rgb * illuminationExposure));
	vec3 ambient = ambientDiffuse * ao;

	//float aoMult = pow(occlusion, 0.5);

	//diffuse *= ao * aoMult;
	//diffuse += ambient * occlusion;
	diffuse += ambient;

	pixelColor = vec4(diffuse, 1.0);

	if (config.debugMode == 1) {pixelColor = vec4(worldNormal * 0.5 + 0.5, 1.0);}
	if (config.debugMode == 2) {pixelColor = vec4(data.N * 0.5 + 0.5, 1.0);}
	if (config.debugMode == 3) {pixelColor = vec4(RandomColor(lod), 1.0);}
}