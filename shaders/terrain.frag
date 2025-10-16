#version 460

#extension GL_ARB_shading_language_include : require

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
} variables;

//layout(set = 1, binding = 0) uniform sampler2D textures[3];

layout(location = 0) in vec3 worldPosition;

layout(location = 0) out vec4 pixelColor;

#include "lighting.glsl"
#include "sampling.glsl"

void main()
{
	//vec3 color = texture(textures[0], worldCoordinate).rgb;
	//vec3 weights = GetWeights(normalize(worldNormal), 1.0);
	//vec3 normal = SampleNormal(textures[1], worldCoordinate, normalize(worldNormal)).rgb;
	//vec3 arm = texture(textures[2], worldCoordinate).rgb;
	//float roughness = arm.g;
	//float metallic = arm.b;
	//float ao = arm.r;

	vec3 color = vec3(0.25, 1.0, 0.25);
	vec3 normal = normalize(vec3(0, 1, 0));
	float roughness = 0.5;
	float metallic = 0.0;
	float ao = 0.0;

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
	diffuse += ambient;

	pixelColor = vec4(diffuse, 1.0);
}