#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED

float rand(vec2 seed)
{
	return (fract(sin(dot(seed.xy, vec2(12.9898, 78.233))) * 43758.5453));
}

uint hash(uint x)
{
    x += (x << 10u);
    x ^= (x >>  6u);
    x += (x <<  3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return (x);
}

float rand01(uint x)
{
	return (float(hash(x)) * (1.0 / 4294967295.0));
}

float rand11(uint x)
{
	return ((rand01(x) - 0.5) * 2.0);
}

#endif