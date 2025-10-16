#version 460

#extension GL_ARB_shading_language_include : require

layout(set = 0, binding = 0) uniform Variables
{
	mat4 view;
	mat4 projection;
	vec4 viewPosition;
	vec4 lightDirection;
} variables;

layout(set = 1, binding = 0) uniform models { mat4 model; } object;

layout(location = 0) in vec3 localPosition;

layout(location = 0) out vec3 worldPosition;

void main()
{
	worldPosition = (object.model * vec4(localPosition, 1.0)).xyz;
	gl_Position = variables.projection * variables.view * object.model * vec4(localPosition, 1.0);
}