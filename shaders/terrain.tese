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

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

//layout(location = 0) in vec3 localPosition;

//layout(location = 0) out vec3 worldPosition;
//layout(location = 1) out vec3 worldNormal;

layout(triangles, fractional_odd_spacing, cw) in;

layout(location = 0) out vec3 worldPosition;
layout(location = 1) out vec3 worldNormal;

#include "noise.glsl"
#include "sampling.glsl"

void main()
{
	vec4 tesselatedPosition = gl_in[0].gl_Position * gl_TessCoord[0] + gl_in[1].gl_Position * gl_TessCoord[1] + gl_in[2].gl_Position * gl_TessCoord[2];
	vec2 uv = (tesselatedPosition.xz / 10000.0) + 0.5;
	vec3 noise = fbm2D_withDeriv(uv + 2, 6, 4.0, 0.2);

	const int power = 3;
	float height = pow(noise.x, power);
	//float hx = power * pow(noise.x, power - 1) * noise.y;
	//float hz = power * pow(noise.x, power - 1) * noise.z;

	//worldNormal = DerivativeToNormal(vec2(hx, hz));
	worldNormal = vec3(0);

	vec3 sampledPosition = tesselatedPosition.xyz;
	sampledPosition.y = (height * 0.5) * 10000.0;

	worldPosition = sampledPosition;

	gl_Position = variables.projection * variables.view * vec4(worldPosition, 1.0);
}