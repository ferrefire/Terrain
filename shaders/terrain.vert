#version 460

#extension GL_ARB_shading_language_include : require

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
	vec4 resolution;
} variables;

//layout(set = 1, binding = 0) uniform sampler2D heightmap;

layout(set = 2, binding = 0) uniform models { mat4 model; } object;

layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

#include "noise.glsl"
#include "sampling.glsl"

void main()
{
	vec2 uv = localPosition.xz + 0.5;
	//float height = texture(heightmap, uv).r;
	
	//vec3 noise = fbm(uv, 5);
	//noise.x = ((noise.x + 0.5) * (noise.x + 0.5) * (noise.x + 0.5)) - 0.5;
	//float height = noise.x;
	//float hx = noise.y;
	//float hz = noise.z;

	vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4.0, 0.2);

	const int power = 3;
	float height = pow(noise.x, power);
	//float hx = power * pow(noise.x, power - 1) * noise.y;
	//float hz = power * pow(noise.x, power - 1) * noise.z;

	//worldNormal = (object.model * vec4(DerivativeToNormal(vec2(hx, hz)), 0.0)).xyz;

	vec3 sampledPosition = localPosition;
	sampledPosition.y += height * 0.5;

	vec3 worldPosition = (object.model * vec4(sampledPosition, 1.0)).xyz;
	gl_Position = vec4(worldPosition, 1.0);
	//gl_Position = variables.projection * variables.view * object.model * vec4(sampledPosition, 1.0);
}