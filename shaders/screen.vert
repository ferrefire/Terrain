#version 460

#extension GL_GOOGLE_include_directive : require

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

layout(location = 0) out vec2 worldCoordinates;

#include "noise.glsl"
#include "sampling.glsl"

void main()
{	
	worldCoordinates = localPosition.xy + 0.5;

    gl_Position = vec4((localPosition.xy * 2.0), 0.0, 1.0);
}