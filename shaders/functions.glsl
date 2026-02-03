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

float Exaggerate(float x)
{
	return (((exp(pow(x * 2.0 - 2.0, 3.0))) + (pow(x, 2.0))) * 0.5);
}

#endif