#ifndef FUCTIONS_INCLUDED
#define FUCTIONS_INCLUDED

vec3 NormalizeSum(vec3 vec)
{
    float sum = abs(vec.x) + abs(vec.y) + abs(vec.z);
    vec /= sum;

    return (vec);
}

float SquaredDistance(vec3 p1, vec3 p2)
{
	vec3 difference = p2 - p1;
	return (dot(difference, difference));
}

float SquaredMagnitude(vec3 p)
{
	return (dot(p, p));
}

float SquaredMagnitude(vec2 p)
{
	return (dot(p, p));
}

float Exaggerate(float x)
{
	return (((exp(pow(x * 2.0 - 2.0, 3.0))) + (pow(x, 2.0))) * 0.5);
}

float InvTrunc(float x)
{
	return (sign(x) * ceil(abs(x)));
}

float easeInOutQuad(float t)
{
	t = clamp(t, 0.0, 1.0);
	return (t < 0.5 ? 2.0 * t * t : 1.0 - pow(-2.0 * t + 2.0, 2.0) / 2.0);
}

float easeInOutCubic(float t)
{
	t = clamp(t, 0.0, 1.0);
	return (t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) / 2.0);
}

#endif