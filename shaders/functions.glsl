#ifndef FUCTIONS_INCLUDED
#define FUCTIONS_INCLUDED

struct Triangle
{
    vec2 p1;
    vec2 p2;
    vec2 p3;
};

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

bool InsideTriangle(Triangle t, vec2 p)
{
    float s1 = (t.p2.x - t.p1.x) * (p.y - t.p1.y) - (t.p2.y - t.p1.y) * (p.x - t.p1.x);
    float s2 = (t.p3.x - t.p2.x) * (p.y - t.p2.y) - (t.p3.y - t.p2.y) * (p.x - t.p2.x);
    float s3 = (t.p1.x - t.p3.x) * (p.y - t.p3.y) - (t.p1.y - t.p3.y) * (p.x - t.p3.x);

    if ((s1 > 0.0 && s2 > 0.0 && s3 > 0.0) || (s1 < 0.0 && s2 < 0.0 && s3 < 0.0)) return (true);
    else return (false);
}

#endif