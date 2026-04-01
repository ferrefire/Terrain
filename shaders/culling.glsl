#ifndef CULLING_INCLUDED
#define CULLING_INCLUDED

#include "transformation.glsl"
#include "functions.glsl"

int InView(vec3 worldSpace, vec3 tolerance)
{
    vec3 clipSpacePosition = WorldToClip(worldSpace);

	bool xInView = clipSpacePosition.x >= 0.0 - tolerance.x && clipSpacePosition.x <= 1.0 + tolerance.x;
	bool yInView = clipSpacePosition.y >= 0.0 - tolerance.y && clipSpacePosition.y <= 1.0 + tolerance.y;
	bool zInView = clipSpacePosition.z >= 0.0 && clipSpacePosition.z <= 1.0;

	return ((xInView && yInView && zInView) ? 1 : 0);
}

# if defined(TERRAIN_FUCTIONS_INCLUDED) && defined(VARIABLES_INCLUDED)

int BehindTerrain(vec3 worldPosition, int iterations, uint ease)
{
	vec3 origin = variables.viewPosition.xyz;
	vec3 rayPosition = origin;
	float inter = 0.0;
	float iterationsMult = 1.0 / float(iterations);
	float terrainHeight = 0.0;
	//float heighest = -maxHeight;
	int result = 0;
	int i = 0;

	while (i <= iterations)
	{
		//if (exponent <= 1.0) {rayPosition = mix(origin, worldPosition, easeInOutQuad(inter));}
		//else {rayPosition = mix(origin, worldPosition, easeInOutCubic(inter));}
		
		float easeInter = inter;
		if (ease == 1) {easeInter = easeInOutQuad(inter);}
		else if (ease == 2) {easeInter = easeInOutCubic(inter);}
		else if (ease == 3) {easeInter = pow(inter, 2);}
		rayPosition = mix(origin, worldPosition, easeInter);

		terrainHeight = TerrainValues(rayPosition.xz).x * maxHeight;

		if (terrainHeight > rayPosition.y + variables.terrainOffset.y * 10000.0)
		{
			result = 1;
			break;
			//heighest = max(heighest, terrainHeight - rayPosition.y);
			//result = 0.0;
		}
		
		inter = i * iterationsMult;
		i++;
	}

	return (result);
}

# endif

#endif