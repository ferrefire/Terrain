#ifndef SAMPLING_INCLUDED
#define SAMPLING_INCLUDED

#include "functions.glsl"

vec3 GetWeights(vec3 normal, float strength)
{
    vec3 weights = abs(normal);
	weights = NormalizeSum(weights);
	
	if (strength != 1.0)
	{
		weights = pow(weights, vec3(strength));
		weights = NormalizeSum(weights);
	}

	return (weights);
}

vec3 UnpackNormal(vec4 packedNormal, float scale)
{
	vec3 normal;

	normal = (packedNormal * 2.0 - 1.0).xyz;
	normal.xy *= scale;
	normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));

	return (normalize(normal));
}

vec3 SampleTriplanarNormalFlat(sampler2D textureSampler, vec2 uv, vec3 weights, vec3 normal, float power)
{
	vec3 tangentX = UnpackNormal(texture(textureSampler, uv), power);
	vec3 tangentY = UnpackNormal(texture(textureSampler, uv), power);
	vec3 tangentZ = UnpackNormal(texture(textureSampler, uv), power);

	tangentX = vec3(tangentX.xy + normal.zy, abs(tangentX.z) * normal.x);
	tangentY = vec3(tangentY.xy + normal.xz, abs(tangentY.z) * normal.y);
	tangentZ = vec3(tangentZ.xy + normal.xy, abs(tangentZ.z) * normal.z);

	vec3 result = normalize(tangentX.zyx * weights.x + tangentY.xzy * weights.y + tangentZ.xyz * weights.z);

	return (result);
}

vec3 SampleNormal(sampler2D textureSampler, vec2 uv, vec3 normal)
{	
	vec3 weights = GetWeights(normal, 1.0);

	vec3 tangentX = UnpackNormal(texture(textureSampler, uv), 2);
	vec3 tangentY = UnpackNormal(texture(textureSampler, uv), 2);
	vec3 tangentZ = UnpackNormal(texture(textureSampler, uv), 2);

	tangentX = vec3(tangentX.xy + normal.zy, abs(tangentX.z) * normal.x);
	tangentY = vec3(tangentY.xy + normal.xz, abs(tangentY.z) * normal.y);
	tangentZ = vec3(tangentZ.xy + normal.xy, abs(tangentZ.z) * normal.z);

	vec3 result = normalize(tangentX.zyx * weights.x + tangentY.xzy * weights.y + tangentZ.xyz * weights.z);

	return (result);
}

vec3 RotateRodriques(vec3 v, vec3 k, float theta) 
{
    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    return (v * cosTheta + cross(k, v) * sinTheta + k * dot(k, v) * (1.0 - cosTheta));
}

vec3 SampleNormalR(sampler2D textureSampler, vec2 uv, vec3 normal, float power)
{
	vec3 tangent = UnpackNormal(texture(textureSampler, uv), power);
    vec3 up = vec3(0.0, 0.0, 1.0);
    vec3 axis = normalize(cross(up, normal));
    float angle = acos(clamp(dot(up, normal), -1.0, 1.0));
	vec3 rotatedNormal = RotateRodriques(tangent, axis, angle);

	return (normalize(rotatedNormal));
}

#endif