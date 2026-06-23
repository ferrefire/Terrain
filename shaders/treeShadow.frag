#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"

layout(location = 0) out vec4 pixelColor;

void main()
{
	pixelColor = vec4(1.0);
}