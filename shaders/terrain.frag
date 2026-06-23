#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

layout(set = 1, binding = 0) uniform sampler2D rockTextures[3];
layout(set = 1, binding = 1) uniform sampler2D grassTextures[3];
layout(set = 1, binding = 2) uniform sampler2D dryTextures[3];
layout(set = 1, binding = 3, std140) readonly uniform terrainShaderData
{
	TerrainShaderData config;
};

layout(location = 0) in vec3 worldPosition;
layout(location = 1) flat in int chunkLod;
layout(location = 2) flat in int chunkId;
//layout(location = 1) in vec3 worldNormal;

layout(location = 0) out vec4 pixelColor;

#include "lighting.glsl"
#include "sampling.glsl"
#include "functions.glsl"
#include "noise.glsl"
#include "terrainFunctions.glsl"

void SampleSteepnessTexture(sampler2D samplers[3], inout TextureData textureData, float strength, float scale, float lodInter, float normalStrength)
{
	if (lodInter <= 0.0) {textureData.color += SampleTriplanarColor(samplers[0], textureData.uv * scale, textureData.weights) * strength;}
	else {textureData.color += SampleTriplanarColorLod(samplers[0], textureData.uv * scale, textureData.weights) * strength;}
	textureData.normal += SampleTriplanarNormal(samplers[1], textureData.uv * scale, textureData.weights, textureData.baseNormal, normalStrength, lodInter) * strength;
	textureData.arm += SampleTriplanarColor(samplers[2], textureData.uv * scale, textureData.weights, vec3(1.0, 1.0, 0.0), lodInter) * strength;
}

void main()
{
	float viewDistance = distance(variables.viewPosition.xyz, worldPosition);

	vec4 terrainValues = TerrainValues(worldPosition.xz);
	vec3 _worldNormal = terrainValues.yzw;

	vec3 triplanarUV = worldPosition + mod(variables.terrainOffset.xyz * terrainDiv, 5000.0);

	float steepness = 1.0 - (dot(_worldNormal, vec3(0, 1, 0)) * 0.5 + 0.5);

	//float lightStrength = 6 * clamp(dot(variables.lightDirection.xyz, vec3(0, 1, 0)) * 3.0, 1.0, 2.0);
	//float lightStrength = 6;
	float lightStrength = 8;
	//float lightStrength = 10;

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * lightStrength;

	//vec3 weights = GetWeights(_worldNormal, 4.0);

	vec3 diffuse = vec3(0);

	//vec3 color = vec3(0.0);
	//vec3 normal = vec3(0.0, 1.0, 0.0);
	//vec3 arm = vec3(1, 1, 0);

	TextureData textureData;
	textureData.color = vec3(0.0);
	textureData.normal = vec3(0.0, 1.0, 0.0);
	textureData.arm = vec3(1, 1, 0);
	textureData.uv = triplanarUV;
	textureData.baseNormal = _worldNormal;
	textureData.weights = GetWeights(_worldNormal, 72);

	//if (steepness <= rockSteepness + steepnessHalfTransition)
	float viewInter = clamp(viewDistance / 10000.0, 0.0, 1.0);

	float snow = pow(clamp(worldPosition.y + (variables.terrainOffset.y * terrainDiv) + config.snowHeight, 0.0, config.snowBlend) / config.snowBlend, 0.75);

	if (steepness <= config.rockSteepness)
	{
		textureData.color *= 0.0;
		textureData.normal *= 0.0;
		textureData.arm *= 0.0;

		SampleSteepnessTexture(grassTextures, textureData, 1.0, 0.5, clamp((viewInter - 0.0075) / 0.0025, 0.0, 1.0), 1.0);
	}

	if (steepness > config.rockSteepness - config.rockTransition)
	{
		float strength = clamp(steepness - (config.rockSteepness - config.rockTransition), 0.0, config.rockTransition) / config.rockTransition;

		textureData.color *= (1.0 - strength);
		textureData.normal *= (1.0 - strength);
		textureData.arm *= (1.0 - strength);

		//diffuse *= 1.0 - strength;
		//diffuse += GetColor(rockTextures, data, weights, _worldNormal, triplanarUV * 0.005, false) * (strength);

		const int scaleCascades = 4;
		const float scales[4] = {0.001, 0.005, 0.05, 0.2};
		const float distances[4] = {0.25, 0.015, 0.0025, 0.0};
		//const float normalStrengths[4] = {1.5, 1.375, 1.25, 1.0};

		if (viewInter > distances[0])
		{
			SampleSteepnessTexture(rockTextures, textureData, strength, scales[0], 0.0, 1.0);
		}
		else
		{
			for (int i = 1; i < scaleCascades; i++)
			{
				if (viewInter > distances[i])
				{
					float blendDistance = distances[i - 1] * 0.5;
					float blendCutoff = distances[i - 1] - blendDistance;
					float scaleStrength = 1.0;
					if (viewInter > blendCutoff)
					{
						scaleStrength = 1.0 - ((viewInter - blendCutoff) / blendDistance);
						SampleSteepnessTexture(rockTextures, textureData, strength * (1.0 - scaleStrength), scales[i - 1], 0.0, 1.0);
					}

					SampleSteepnessTexture(rockTextures, textureData, strength * scaleStrength, scales[i], 0.0, 1.0);
					break;
				}
			}
		}
	}

	if (config.snowEnabled == 1 && snow > 0.0)
	{
		float coverSteepness = mix(0.0, config.snowSteepness, snow);
		float diff = coverSteepness - steepness;
		//coverSteepness = mix(0.0, snowSteepness, snow);
		//diff = coverSteepness - steepness;
		if (diff > 0.05)
		{
			textureData.color = ToLinear(vec3(1.0));
		}
		else if (diff > 0.0)
		{
			float diffBlend = diff / 0.05;
			textureData.color = mix(textureData.color, ToLinear(vec3(1.0)), diffBlend);
		}
		//textureData.color = mix(textureData.color, vec3(1.0), snow);
	}

	if (config.debugMode == 3 || config.debugMode == 4)
	{
		float cascadeDebug = TerrainCascadeLod(worldPosition.xz);
		if (config.debugMode == 3 && mod(cascadeDebug, 1.0) > 0.99) {textureData.color = RandomColor(int(floor(cascadeDebug)));}
		else if (config.debugMode == 4) {textureData.color = RandomColor(int(floor(cascadeDebug)));}
		//color = RandomColor(chunkLod);
	}

	if (config.debugMode == 5) {textureData.color = RandomColor(chunkLod);}
	if (config.debugMode == 6) {textureData.color = RandomColor(chunkId);}
	if (config.debugMode == 7)
	{
		textureData.color = RandomColor(1);
		if (steepness <= config.rockSteepness - config.rockTransition) {textureData.color = RandomColor(2);}
		else if (steepness <= config.rockSteepness) {textureData.color = RandomColor(3);}
		else {textureData.color = RandomColor(4);}
	}

	float roughness = textureData.arm.g;
	float metallic = textureData.arm.b;
	float ao = textureData.arm.r;

	data.N = normalize(textureData.normal);
	//data.V = normalize(variables.viewPosition.xyz - worldPosition);
	//data.L = variables.lightDirection.xyz;
	data.albedo = textureData.color;
	data.metallic = metallic;
	data.roughness = roughness;

	diffuse = PBRLighting(data);

	//vec3 ambientDiffuse = 0.15 * textureData.color * vec3(1.0, 0.9, 0.7);
	//vec3 ambientDiffuse = 0.1 * textureData.color * vec3(1.0, 0.9, 0.7) * lightStrength;
	//vec3 ambient = ambientDiffuse * ao;
	
	float shadow = TerrainShadow(vec3(worldPosition.x, -maxHeight * 0.5, worldPosition.z));
	if (shadow > 0.0) {shadow = min(shadow, SampleShadows(worldPosition));}
	diffuse *= shadow;

	vec3 illumination = TerrainIllumination(worldPosition, _worldNormal);
	float occlusion = TerrainOcclusion(worldPosition.xz);

	vec3 ambientDiffuse = 0.25 * (textureData.color * illumination.rgb);
	vec3 ambient = ambientDiffuse * ao;

	float aoMult = pow(occlusion, 0.5);
	//float aoMult = 1.0;

	diffuse *= ao * aoMult;
	diffuse += ambient * occlusion;

	vec3 finalColor = diffuse;

	pixelColor = vec4(finalColor, 1.0);
	if (config.debugMode == 1) {pixelColor = vec4(_worldNormal * 0.5 + 0.5, 1.0);}
	if (config.debugMode == 2) {pixelColor = vec4(data.N * 0.5 + 0.5, 1.0);}
}