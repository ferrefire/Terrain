#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED

float rand(vec2 seed)
{
	return (fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453));
}

#endif