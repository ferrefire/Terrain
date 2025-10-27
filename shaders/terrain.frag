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

layout(set = 1, binding = 0) uniform sampler2D rockTextures[3];
layout(set = 1, binding = 1) uniform sampler2D grassTextures[3];

layout(location = 0) in vec3 worldPosition;
layout(location = 1) flat in int chunkLod;
//layout(location = 1) in vec3 worldNormal;

layout(location = 0) out vec4 pixelColor;

#include "lighting.glsl"
#include "sampling.glsl"
#include "functions.glsl"
#include "noise.glsl"
#include "terrainFunctions.glsl"

vec3 GetColor(sampler2D samplers[3], PBRInput data, vec3 weights, vec3 _worldNormal, vec3 triplanarUV, bool lod)
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

	PBRInput data;
	data.V = normalize(variables.viewPosition.xyz - worldPosition);
	data.L = normalize(variables.lightDirection.xyz);
	data.lightColor = vec3(1.0, 0.9, 0.7) * 4;

	vec3 weights = GetWeights(_worldNormal, 2.0);

	vec3 diffuse = vec3(0);
	if (steepness <= 0.18)
	{
		float strength = 1.0 - clamp(steepness - 0.12, 0.0, 0.06) / 0.06;
		//diffuse += GetColor(grassTextures, _worldNormal, triplanarUV, viewDistance > 1000.0) * strength;
		diffuse += GetColor(grassTextures, data, weights, _worldNormal, triplanarUV, viewDistance > 1000.0) * strength;
	}
	if (steepness >= 0.12)
	{
		float strength = 1.0 - clamp(0.18 - steepness, 0.0, 0.06) / 0.06;
		//diffuse += GetColor(rockTextures, _worldNormal, triplanarUV * 0.005, false) * strength;
		diffuse += GetColor(rockTextures, data, weights, _worldNormal, triplanarUV * 0.005, false) * strength;
	}

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

	
	//float fog = exp(-(viewDistance / 10000.0));
	//float fog = viewDistance / 10000.0;
	float fog = viewDistance / 30000.0;
	//vec3 finalColor = mix(diffuse, vec3(0.75), clamp(pow(1.0 - exp(-fog), 3.0), 0.0, 1.0));
	vec3 finalColor = mix(diffuse, vec3(0.75), clamp(1.0 - exp(-fog), 0.0, 1.0));
	//vec3 finalColor = diffuse;

	//float fog = viewDistance / variables.resolution.w;
	//vec3 finalColor = mix(diffuse, vec3(0.75), clamp(pow(fog, 1), 0.0, 1.0));

	//int total = int(floor(abs(worldPosition.x) * 0.01) * 100 + floor(abs(worldPosition.z) * 0.01) * 100);

	//finalColor *= (total % 1000 == 0 ? 0.0 : 1.0);

	pixelColor = vec4(finalColor, 1.0);
	//pixelColor = vec4(normal * 0.5 + 0.5, 1.0);
	//pixelColor = vec4((texture(textures[1], worldPosition.xz * vec2(0.25, 0.25)).rgb ), 1.0);

	//if (worldPosition.z > 0 && abs(worldPosition.x) < 100) pixelColor = vec4(0, 0, 1, 1);
	//if (worldPosition.x > 0 && abs(worldPosition.z) < 100) pixelColor = vec4(0, 1, 0, 1);
}