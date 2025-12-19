#version 460

#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec3 localPosition;

layout(location = 0) out vec2 worldCoordinates;

void main()
{	
	worldCoordinates = localPosition.xy + 0.5;

    gl_Position = vec4((localPosition.xy * 2.0), 0.0, 1.0);
}