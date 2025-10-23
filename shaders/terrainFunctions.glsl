#ifndef TERRAIN_FUCTIONS_INCLUDED
#define TERRAIN_FUCTIONS_INCLUDED

layout(set = 0, binding = 1) uniform sampler2D heightmaps[8];

vec4 TerrainValues(vec2 worldPosition)
{
	vec2 uv;
	vec4 heightData;

	for (int i = 0; i < 8; i++)
	{
		uv = worldPosition / 10000.0 - (variables.heightmapOffsets[i].xy - variables.terrainOffset.xz);
		float size = 0.05 * pow(2.0, i);
		if (max(abs(uv.x), abs(uv.y)) < (size * 0.5))
		{
			uv = uv / size;
			heightData = texture(heightmaps[i], uv + 0.5).rgba;
			vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
			return (vec4(heightData.x, terrainNormal));
		}
	}

	return (vec4(0, 0, 1, 0));

	/*uv = worldPosition / 10000.0 - (variables.heightmapOffsets[0].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.025)
	{
		uv = uv / 0.05;
		heightData = texture(heightmaps[0], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[1].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.05)
	{
		uv = uv / 0.1;
		heightData = texture(heightmaps[1], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[2].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.1)
	{
		uv = uv / 0.2;
		heightData = texture(heightmaps[2], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[3].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.2)
	{
		uv = uv / 0.4;
		heightData = texture(heightmaps[3], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[4].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.4)
	{
		uv = uv / 0.8;
		heightData = texture(heightmaps[4], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[5].xy - variables.terrainOffset.xz);
	if (max(abs(uv.x), abs(uv.y)) < 0.8)
	{
		uv = uv / 1.6;
		heightData = texture(heightmaps[5], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	uv = worldPosition / 10000.0 - (variables.heightmapOffsets[6].xy - variables.terrainOffset.xz);

	{
		uv = uv / 3.2;
		heightData = texture(heightmaps[6], uv + 0.5).rgba;
		vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
		return (vec4(heightData.x, terrainNormal));
	}

	//vec3 terrainNormal = normalize(heightData.gba * 2.0 - 1.0);
	//return (vec4(heightData.x, terrainNormal));*/
}

#endif