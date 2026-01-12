#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

layout(set = 1, binding = 0) uniform sampler2D rockTextures[3];
layout(set = 1, binding = 1) uniform sampler2D grassTextures[3];
layout(set = 1, binding = 2) uniform sampler2D dryTextures[3];

layout(location = 0) in vec3 worldPosition;
layout(location = 1) flat in int chunkLod;
//layout(location = 1) in vec3 worldNormal;

layout(location = 0) out vec4 pixelColor;

#include "lighting.glsl"
#include "sampling.glsl"
#include "functions.glsl"
#include "noise.glsl"
#include "terrainFunctions.glsl"

const float rockSteepness = 0.10;
const float drySteepness = 0.1;
const float rockTransition = 0.075;
const float dryTransition = 0.025;
//const float steepnessHalfTransition = steepnessTransition * 0.5;

/*vec3 GetColor(sampler2D samplers[3], PBRInput data, vec3 weights, vec3 _worldNormal, vec3 triplanarUV, bool lod)
{
	//vec3 weights = GetWeights(_worldNormal, 2.0);
	vec3 color = SampleTriplanarColor(samplers[0], triplanarUV, weights);
	vec3 normal = _worldNormal;
	if (!lod) normal = SampleTriplanarNormal(samplers[1], triplanarUV, weights, _worldNormal, 1.0);
	vec3 arm = vec3(1, 1, 0);
	if (!lod) arm = SampleTriplanarColor(samplers[2], triplanarUV, weights);
	float roughness = arm.g;
	float metallic = arm.b;
	float ao = arm.r;

	if (variables.terrainOffset.w > 0)
	{
		float cascadeDebug = TerrainCascadeLod(worldPosition.xz);
		if (variables.terrainOffset.w == 1 && mod(cascadeDebug, 1.0) > 0.99) {color = RandomColor(int(floor(cascadeDebug)));}
		else if (variables.terrainOffset.w == 2) {color = RandomColor(int(floor(cascadeDebug)));}
		//color = RandomColor(chunkLod);
	}

	//PBRInput data;
	data.N = normal;
	//data.V = normalize(variables.viewPosition.xyz - worldPosition);
	//data.L = variables.lightDirection.xyz;
	data.albedo = color;
	data.metallic = metallic;
	data.roughness = roughness;
	//data.lightColor = vec3(1.0, 0.9, 0.7) * 4;

	vec3 diffuse = PBRLighting(data);

	vec3 ambientDiffuse = 0.15 * color * vec3(1.0, 0.9, 0.7);
	vec3 ambient = ambientDiffuse * ao;
	diffuse += ambient;

	return (diffuse);
}*/

struct TextureData
{
	vec3 color;
	vec3 normal;
	vec3 arm;
	vec3 weights;
	vec3 uv;
	vec3 baseNormal;
};

void SampleSteepnessTexture(sampler2D samplers[3], inout TextureData textureData, float strength, float scale, float lodInter)
{
	//float inverseStrength = (1.0 - strength);
	//textureData.color *= inverseStrength;
	//textureData.normal *= inverseStrength;
	//textureData.arm *= inverseStrength;

	if (lodInter <= 0.0) {textureData.color += SampleTriplanarColor(samplers[0], textureData.uv * scale, textureData.weights) * strength;}
	else {textureData.color += SampleTriplanarColorLod(samplers[0], textureData.uv * scale, textureData.weights) * strength;}
	textureData.normal += SampleTriplanarNormal(samplers[1], textureData.uv * scale, textureData.weights, textureData.baseNormal, 1.0, lodInter) * strength;
	textureData.arm += SampleTriplanarColor(samplers[2], textureData.uv * scale, textureData.weights, vec3(1.0, 1.0, 0.0), lodInter) * strength;
}

void main()
{
	//vec2 uv = (worldPosition.xz / 10000.0) + variables.terrainOffset.xz;
	//vec2 uv = (worldPosition.xz / 5000.0) + 0.5;
	//vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4, 0.2);
	//vec3 noise = fbm(uv + 2, 6, 3.75, 0.2);

	//const int power = 3;
	//float height = pow(noise.x, power);
	//float hx = power * pow(noise.x, power - 1) * noise.y;
	//float hz = power * pow(noise.x, power - 1) * noise.z;

	float viewDistance = distance(variables.viewPosition.xyz, worldPosition);
	//float qualityInter = clamp(viewDistance, 0.0, 2500.0) / 2500.0;
	//int qualityDescrease = int(round(mix(0, 5, qualityInter)));

	//vec3 tnoise = TerrainData(uv, int(variables.terrainOffset.w) - qualityDescrease, false);
	//vec3 _worldNormal = DerivativeToNormal(vec2(tnoise.y, tnoise.z));

	//vec4 heightData = texture(heightmaps[1], uv).rgba;
	//vec3 _worldNormal = normalize(heightData.gba * 2.0 - 1.0);

	vec3 _worldNormal = TerrainValues(worldPosition.xz).yzw;
	//if (viewDistance > 75.0) {_worldNormal = TerrainValues(worldPosition.xz).yzw;}
	//else {_worldNormal = normalize(worldNormal);}

	//vec3 _worldNormal = worldNormal;
	vec3 triplanarUV = worldPosition + mod(variables.terrainOffset.xyz * 10000.0, 5000.0);
	//vec3 triplanarUV = worldPosition;

	float steepness = 1.0 - (dot(_worldNormal, vec3(0, 1, 0)) * 0.5 + 0.5);

	//float lightStrength = 6 * clamp(dot(variables.lightDirection.xyz, vec3(0, 1, 0)) * 3.0, 1.0, 2.0);
	float lightStrength = 6;
	//float lightStrength = 8;

	//if (variables.glillOffsets[0].y == 1) {lightStrength = 8;}

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
	textureData.weights = GetWeights(_worldNormal, 4.0);

	//if (steepness <= rockSteepness + steepnessHalfTransition)
	float viewInter = clamp(viewDistance / 10000.0, 0.0, 1.0);
	
	//if (steepness <= drySteepness)
	if (steepness <= rockSteepness)
	{
		//diffuse = GetColor(grassTextures, data, weights, _worldNormal, triplanarUV, false);

		//color = SampleTriplanarColor(grassTextures[0], triplanarUV, weights);
		//normal = SampleTriplanarNormal(grassTextures[1], triplanarUV, weights, _worldNormal, 1.0);
		//arm = SampleTriplanarColor(grassTextures[2], triplanarUV, weights);

		textureData.color *= 0.0;
		textureData.normal *= 0.0;
		textureData.arm *= 0.0;

		SampleSteepnessTexture(grassTextures, textureData, 1.0, 0.5, clamp((viewInter - 0.0075) / 0.0025, 0.0, 1.0));
	}
	/*if (steepness > drySteepness - dryTransition && steepness <= rockSteepness)
	{
		float strength = clamp(steepness - (drySteepness - dryTransition), 0.0, dryTransition) / dryTransition;
		//diffuse *= 1.0 - strength;
		//diffuse += GetColor(dryTextures, data, weights, _worldNormal, triplanarUV * 0.1, false) * (strength);

		float scale = 0.1;
		if (viewInter < 0.0025) scale = 0.25;

		//color *= 1.0 - strength;
		//normal *= 1.0 - strength;
		//arm *= 1.0 - strength;
		//color += SampleTriplanarColor(dryTextures[0], triplanarUV * scale, weights) * strength;
		//normal += SampleTriplanarNormal(dryTextures[1], triplanarUV * scale, weights, _worldNormal, 1.0) * strength;
		//arm += SampleTriplanarColor(dryTextures[2], triplanarUV * scale, weights) * strength;

		SampleSteepnessTexture(dryTextures, textureData, strength, scale);
	}*/
	if (steepness > rockSteepness - rockTransition)
	{
		float strength = clamp(steepness - (rockSteepness - rockTransition), 0.0, rockTransition) / rockTransition;

		textureData.color *= (1.0 - strength);
		textureData.normal *= (1.0 - strength);
		textureData.arm *= (1.0 - strength);

		//diffuse *= 1.0 - strength;
		//diffuse += GetColor(rockTextures, data, weights, _worldNormal, triplanarUV * 0.005, false) * (strength);

		const int scaleCascades = 4;
		const float scales[4] = {0.001, 0.005, 0.05, 0.2};
		const float distances[4] = {0.25, 0.015, 0.0025, 0.0};

		if (viewInter > distances[0])
		{
			SampleSteepnessTexture(rockTextures, textureData, strength, scales[0], 0.0);
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
						SampleSteepnessTexture(rockTextures, textureData, strength * (1.0 - scaleStrength), scales[i - 1], 0.0);
					}

					SampleSteepnessTexture(rockTextures, textureData, strength * scaleStrength, scales[i], 0.0);
					break;
				}
			}
		}

		/*float scale = 0;
		float scaleInter = 0.0;
		if (viewInter < 0.0025)
		{
			scale = 0.2;
		}
		else if (viewInter < 0.0075)
		{
			scale = 0.1;
		}
		else if (viewInter < 0.25)
		{
			scale = 0.005;
		}
		else
		{
			scale = 0.001;
		}*/
		

		//color *= 1.0 - strength;
		//normal *= 1.0 - strength;
		//arm *= 1.0 - strength;
		//color += SampleTriplanarColor(rockTextures[0], triplanarUV * scale, weights) * strength;
		//normal += SampleTriplanarNormal(rockTextures[1], triplanarUV * scale, weights, _worldNormal, 1.0) * strength;
		//arm += SampleTriplanarColor(rockTextures[2], triplanarUV * scale, weights) * strength;

		//SampleSteepnessTexture(rockTextures, textureData, strength, scale, 0.0);
	}

	if (variables.terrainOffset.w > 0)
	{
		float cascadeDebug = TerrainCascadeLod(worldPosition.xz);
		if (variables.terrainOffset.w == 1 && mod(cascadeDebug, 1.0) > 0.99) {textureData.color = RandomColor(int(floor(cascadeDebug)));}
		else if (variables.terrainOffset.w == 2) {textureData.color = RandomColor(int(floor(cascadeDebug)));}
		//color = RandomColor(chunkLod);
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
	
	float shadow = TerrainShadow(vec3(worldPosition.x, -2500.0, worldPosition.z));
	diffuse *= shadow;

	vec3 illumination = TerrainIllumination(worldPosition, _worldNormal);
	//vec4 illumination = TerrainIllumination(worldPosition, _worldNormal);
	//float occlusion = illumination.w;
	float occlusion = TerrainOcclusion(worldPosition.xz);

	vec3 ambientDiffuse = 0.25 * textureData.color * illumination.rgb;
	vec3 ambient = ambientDiffuse * ao;

	float aoMult = pow(occlusion, 0.5);
	//float aoMult = 1.0;

	//if (variables.glillOffsets[0].y == 1) {aoMult =  pow(mix(0.9, 1.0, occlusion), 8.0);}
	//if (variables.glillOffsets[0].y == 1) {aoMult = pow(occlusion, 0.75);}

	diffuse *= ao * aoMult;
	diffuse += ambient * occlusion;

	//diffuse += ambient;
	//diffuse += ambient * occlusion;
	//diffuse *= ao;

	/*if (steepness <= rockSteepness + steepnessHalfTransition)
	{
		float strength = 1.0 - clamp(steepness - (rockSteepness - steepnessHalfTransition), 0.0, steepnessTransition) / steepnessTransition;
		//diffuse += GetColor(grassTextures, _worldNormal, triplanarUV, viewDistance > 1000.0) * strength;
		//diffuse += GetColor(grassTextures, data, weights, _worldNormal, triplanarUV, viewDistance > 1000.0) * strength;
		diffuse += GetColor(grassTextures, data, weights, _worldNormal, triplanarUV, false) * strength;
	}
	if (steepness >= rockSteepness - steepnessHalfTransition)
	{
		float strength = 1.0 - clamp((rockSteepness + steepnessHalfTransition) - steepness, 0.0, steepnessTransition) / steepnessTransition;
		//diffuse += GetColor(rockTextures, _worldNormal, triplanarUV * 0.005, false) * strength;
		diffuse += GetColor(rockTextures, data, weights, _worldNormal, triplanarUV * 0.005, false) * strength;
	}*/

	//if (steepness <= 0.15)
	//{
	//	diffuse = GetColor(grassTextures, _worldNormal, triplanarUV);
	//}
	//else if (steepness > 0.15)
	//{
	//	diffuse = GetColor(rockTextures, _worldNormal, triplanarUV * 0.005);
	//}

	/*int total = int(floor(abs(worldPosition.x)) + floor(abs(worldPosition.z)));

	vec3 color = vec3(0.25, 1.0, 0.25);
	if (SquaredDistance(worldPosition, variables.viewPosition.xyz) < 100 * 100) color *= (total % 2 == 0 ? 1 : 0.5);
	color *= ((total / 100) % 2 == 0 ? 1 : 0.5);
	color *= ((total / 1000) % 2 == 0 ? 1 : 0.5);

	if (dot(normalize(worldPosition.xz), normalize(variables.lightDirection.xz)) > 0.999) color = vec3(1, 0, 0);

	//vec3 normal = normalize(vec3(0, 1, 0));
	vec3 normal = worldNormal;
	float roughness = 0.5;
	float metallic = 0.0;
	float ao = 0.4;

	PBRInput data;
	data.N = normal;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = variables.lightDirection.xyz;
	data.albedo = color;
	data.metallic = metallic;
	data.roughness = roughness;
	data.lightColor = vec3(1.0, 0.9, 0.7) * 4;

	vec3 diffuse = PBRLighting(data);

	vec3 ambientDiffuse = 0.1 * color * vec3(1.0, 0.9, 0.7);
	vec3 ambient = ambientDiffuse * ao;
	diffuse += ambient;*/

	//int centerDistance = int(length(worldPosition));
	//if (centerDistance % 1000 <= 10) diffuse *= 0.0;

	
	////float fog = exp(-(viewDistance / 10000.0));
	////float fog = viewDistance / 10000.0;
	//float fog = viewDistance / 30000.0;
	////vec3 finalColor = mix(diffuse, vec3(0.75), clamp(pow(1.0 - exp(-fog), 3.0), 0.0, 1.0));
	//vec3 finalColor = mix(diffuse, vec3(0.75), clamp(1.0 - exp(-fog), 0.0, 1.0));
	////vec3 finalColor = diffuse;

	//float fog = viewDistance / variables.resolution.w;
	//vec3 finalColor = mix(diffuse, vec3(0.75), clamp(pow(fog, 1), 0.0, 1.0));

	//int total = int(floor(abs(worldPosition.x) * 0.2) * 5 + floor(abs(worldPosition.z) * 0.2) * 5);
	//finalColor *= (total % 2 == 0 ? 1.0 : 0.75);
	vec3 finalColor = diffuse;

	//float occlusion = TerrainIllumination(worldPosition.xz);
	//if (occlusion < 0.9) {occlusion = 0;}
	//finalColor = vec3(occlusion);

	pixelColor = vec4(finalColor, 1.0);
	//pixelColor = vec4(normal * 0.5 + 0.5, 1.0);
	//pixelColor = vec4((texture(textures[1], worldPosition.xz * vec2(0.25, 0.25)).rgb ), 1.0);

	//if (worldPosition.z > 0 && abs(worldPosition.x) < 100) pixelColor = vec4(0, 0, 1, 1);
	//if (worldPosition.x > 0 && abs(worldPosition.z) < 100) pixelColor = vec4(0, 1, 0, 1);
}