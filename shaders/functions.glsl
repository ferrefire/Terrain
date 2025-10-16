#ifndef FUCTIONS_INCLUDED
#define FUCTIONS_INCLUDED

vec3 NormalizeSum(vec3 vec)
{
    float sum = abs(vec.x) + abs(vec.y) + abs(vec.z);
    vec /= sum;

    return (vec);
}

#endif