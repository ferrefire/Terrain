#ifndef CULLING_INCLUDED
#define CULLING_INCLUDED

#include "transformation.glsl"

int InView(vec3 worldSpace, vec3 tolerance)
{
    vec3 clipSpacePosition = WorldToClip(worldSpace);

	bool xInView = clipSpacePosition.x >= 0.0 - tolerance.x && clipSpacePosition.x <= 1.0 + tolerance.x;
	bool yInView = clipSpacePosition.y >= 0.0 - tolerance.y && clipSpacePosition.y <= 1.0 + tolerance.y;
	bool zInView = clipSpacePosition.z >= 0.0 && clipSpacePosition.z <= 1.0;

	return ((xInView && yInView && zInView) ? 1 : 0);
}

#endif