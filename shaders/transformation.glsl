#ifndef TRANSFORMATION_INCLUDED
#define TRANSFORMATION_INCLUDED

vec3 WorldToClip(vec3 worldSpace)
{
    vec4 viewSpace = variables.projection * variables.view * vec4(worldSpace, 1.0);

    vec3 clipSpace = viewSpace.xyz;
    clipSpace /= viewSpace.w;

    clipSpace.xy = clipSpace.xy * 0.5 + 0.5;
    clipSpace.z = viewSpace.w / variables.resolution.w;

    return (clipSpace);
}

/*vec3 ScreenToWorld(vec2 screenPosition, float depth)
{
	vec2 NDC = screenPosition * 2.0 - 1.0;
	//NDC.y = -NDC.y;

	vec4 clip = vec4(NDC, depth, 1.0);

	mat4 invViewProjMat = inverse(variables.projection * variables.view);

	vec4 worldH = invViewProjMat * clip;

	return (worldH.xyz / worldH.w);
}*/

float LinearizeDepth(float depth)
{
	return ((variables.resolution.z * variables.resolution.w) / (variables.resolution.w - depth * (variables.resolution.w - variables.resolution.z)));
}

float LinearizeDepth01(float depth)
{
	return (((variables.resolution.z * variables.resolution.w) / (variables.resolution.w - depth * (variables.resolution.w - variables.resolution.z))) / variables.resolution.w);
}

float GetDepth(float z)
{
    float depth = z;

    depth = depth * 2.0 - 1.0;
    depth = (2.0 * variables.resolution.z * variables.resolution.w) / (variables.resolution.w + variables.resolution.z - depth * (variables.resolution.w - variables.resolution.z));
    depth = depth / variables.resolution.w;
	//depth = clamp(depth, 0.0, 1.0);

    return (depth);
}

vec3 RotateY(vec3 pos, float deg)
{
	float r = radians(deg);
	float c = cos(r);
	float s = sin(r);

	mat3 rot = mat3(c, 0.0, s,
					0.0, 1.0, 0.0,
					-s, 0.0, c);

	return (rot * pos);
}

vec3 RotateY(vec3 pos, float c, float s)
{
	mat3 rot = mat3(c, 0.0, s,
					0.0, 1.0, 0.0,
					-s, 0.0, c);

	return (rot * pos);
}

vec3 RotateX(vec3 pos, float c, float s)
{
    mat3 rot = mat3(1.0, 0.0, 0.0,
				0.0,  c, -s,
				0.0,  s,  c);

	return (rot * pos);
}

vec3 RotateZ(vec3 pos, float c, float s)
{
	mat3 rot = mat3(c, -s, 0.0,
				s,  c, 0.0,
				0.0, 0.0, 1.0);

	return (rot * pos);
}

#endif