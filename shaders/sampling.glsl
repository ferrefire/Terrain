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

vec3 SampleTriplanarColor(sampler2D textureSampler, vec3 uv, vec3 weights)
{
	vec2 uvy = uv.xz;
	vec2 uvz = uv.xy;
	vec2 uvx = uv.zy;

	//if (rotate)
	//{
	//	uvy = uvy.yx;
	//	uvz = uvz.yx;
	//	uvx = uvx.yx;
	//}

	uvx.y *= -1;
	uvy.y *= -1;
	uvz.y *= -1;

	vec3 xz = vec3(0.0);
	vec3 xy = vec3(0.0);
	vec3 zy = vec3(0.0);

	if (weights.y <= 0.01) {weights.y = 0;}
	if (weights.z <= 0.01) {weights.z = 0;}
	if (weights.x <= 0.01) {weights.x = 0;}

	weights = NormalizeSum(weights);

	if (weights.y > 0.01) xz = (texture(textureSampler, uvy).rgb);
	if (weights.z > 0.01) xy = (texture(textureSampler, uvz).rgb);
	if (weights.x > 0.01) zy = (texture(textureSampler, uvx).rgb);

	vec3 result = xz * weights.y + xy * weights.z + zy * weights.x;

	return (result);
}

vec3 SampleTriplanarColor(sampler2D textureSampler, vec3 uv, vec3 weights, vec3 lodColor, float lodInter)
{
	if (lodInter >= 1.0) {return (lodColor);}

	vec3 result = SampleTriplanarColor(textureSampler, uv, weights);

	if (lodInter <= 0.0) {return (result);}

	return (mix(result, lodColor, clamp(lodInter, 0.0, 1.0)));
}

vec3 SampleTriplanarColorLod(sampler2D textureSampler, vec3 uv, vec3 weights)
{
	if (weights.y > weights.x && weights.y > weights.z) {weights = vec3(0.0, 1.0, 0.0);}
	else if (weights.x > weights.y && weights.x > weights.z) {weights = vec3(1.0, 0.0, 0.0);}
	else if (weights.z > weights.y && weights.z > weights.x) {weights = vec3(0.0, 0.0, 1.0);}

	return (SampleTriplanarColor(textureSampler, uv, weights));
}

vec3 SampleTriplanarNormal(sampler2D textureSampler, vec3 uv, vec3 weights, vec3 normal, float power, float lodInter)
{
	vec2 uvx = uv.zy;
	vec2 uvy = uv.xz;
	vec2 uvz = uv.xy;

	//if (rotate)
	//{
	//	uvy = uvy.yx;
	//	uvz = uvz.yx;
	//	uvx = uvx.yx;
	//}

	uvx.y *= -1;
	uvy.y *= -1;
	uvz.y *= -1;

	vec3 tangentX = vec3(0.0, 0.0, 1.0);
	vec3 tangentY = vec3(0.0, 0.0, 1.0);
	vec3 tangentZ = vec3(0.0, 0.0, 1.0);

	if (weights.x <= 0.01) {weights.x = 0;}
	if (weights.y <= 0.01) {weights.y = 0;}
	if (weights.z <= 0.01) {weights.z = 0;}

	weights = NormalizeSum(weights);

	if (lodInter < 1.0)
	{
		if (weights.x > 0.01) {tangentX = UnpackNormal(vec4(texture(textureSampler, uvx).rg, 0.0, 0.0), power);}
		if (weights.y > 0.01) {tangentY = UnpackNormal(vec4(texture(textureSampler, uvy).rg, 0.0, 0.0), power);}
		if (weights.z > 0.01) {tangentZ = UnpackNormal(vec4(texture(textureSampler, uvz).rg, 0.0, 0.0), power);}
		//if (weights.x > 0.01) {tangentX = UnpackNormal(vec4(texture(textureSampler, uvx).rgb, 0.0), power);}
		//if (weights.y > 0.01) {tangentY = UnpackNormal(vec4(texture(textureSampler, uvy).rgb, 0.0), power);}
		//if (weights.z > 0.01) {tangentZ = UnpackNormal(vec4(texture(textureSampler, uvz).rgb, 0.0), power);}

		if (lodInter > 0.0)
		{
			tangentX = mix(tangentX, vec3(0.0, 0.0, 1.0), lodInter);
			tangentY = mix(tangentY, vec3(0.0, 0.0, 1.0), lodInter);
			tangentZ = mix(tangentZ, vec3(0.0, 0.0, 1.0), lodInter);
		}
	}

	tangentX = vec3(tangentX.xy + normal.zy, abs(tangentX.z) * normal.x);
	tangentY = vec3(tangentY.xy + normal.xz, abs(tangentY.z) * normal.y);
	tangentZ = vec3(tangentZ.xy + normal.xy, abs(tangentZ.z) * normal.z);

	vec3 result = normalize(tangentX.zyx * weights.x + tangentY.xzy * weights.y + tangentZ.xyz * weights.z);

	return (result);
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

	vec3 tangentX = UnpackNormal(texture(textureSampler, uv), 1);
	vec3 tangentY = UnpackNormal(texture(textureSampler, uv), 1);
	vec3 tangentZ = UnpackNormal(texture(textureSampler, uv), 1);

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