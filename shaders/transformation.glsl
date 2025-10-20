#ifndef TRANSFORMATION_INCLUDED
#define TRANSFORMATION_INCLUDED

vec3 WorldToClip(vec3 worldSpace)
{
    vec4 viewSpace = variables.projection * variables.view * vec4(worldSpace, 1.0);

    vec3 clipSpace = viewSpace.xyz;
    clipSpace /= viewSpace.w;

    clipSpace.xy = clipSpace.xy * 0.5 + 0.5;
    clipSpace.z = viewSpace.w * variables.resolution.w;

    return (clipSpace);
}

#endif