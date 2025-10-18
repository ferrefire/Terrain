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

layout(set = 1, binding = 0) uniform sampler2D screenTexture;

//layout(set = 2, binding = 0) uniform models { mat4 model; } object;

layout(location = 0) in vec2 worldCoordinates;

layout(location = 0) out vec4 pixelColor;

void main()
{
	vec3 color = texture(screenTexture, worldCoordinates).rrr;
	
	pixelColor = vec4(color, 1.0);
}