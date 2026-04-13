#version 460

#extension GL_GOOGLE_include_directive : require

#include "variables.glsl"
//#include "functions.glsl"

//layout(location = 0) in vec3 localCoords;

layout(location = 0) out vec4 pixelColor;

/*const vec4 discardDistances = vec4(0.125 * 1.1, 0.1875, 0.375 * 0.9, 0.0625 * 1.75);

const Triangle discardTriangle0 = Triangle(vec2(0.0, 0.25) * 1.5, vec2(0.25, -0.125) * 1.5, vec2(-0.25, -0.125) * 1.5);
const Triangle discardTriangle1 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(-discardDistances.x, discardDistances.y) * 1.425, vec2(-discardDistances.z, discardDistances.w) * 1.425);
const Triangle discardTriangle2 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(-discardDistances.x, -discardDistances.y) * 1.425, vec2(-discardDistances.z, -discardDistances.w) * 1.425);
const Triangle discardTriangle3 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(discardDistances.x, discardDistances.y) * 1.425, vec2(discardDistances.z, discardDistances.w) * 1.425);
const Triangle discardTriangle4 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(discardDistances.x, -discardDistances.y) * 1.425, vec2(discardDistances.z, -discardDistances.w) * 1.425);
const Triangle discardTriangle5 = Triangle(vec2(0.0, 0.0) * 1.425, vec2(0.25, 0.25) * 1.425, vec2(-0.25, 0.25) * 1.425);

bool ShouldDiscard(vec2 position)
{
	if (InsideTriangle(discardTriangle1, position)) return (true);
	else if (InsideTriangle(discardTriangle2, position)) return (true);
	else if (InsideTriangle(discardTriangle3, position)) return (true);
	else if (InsideTriangle(discardTriangle4, position)) return (true);
	else return (false);
}*/

void main()
{
	//if (!ShouldDiscard(localCoords.xy)) {discard;}

	pixelColor = vec4(1.0);
}