#ifndef PACKING_INCLUDED
#define PACKING_INCLUDED

vec2 packFloatToRG8(float v)
{
    v = clamp(v, 0.0, 1.0);
    vec2 enc = vec2(1.0, 255.0) * v;
    enc = fract(enc);
    enc.x -= enc.y * (1.0 / 255.0); // fix carry
    return (enc);
}

float unpackRG8ToFloat(vec2 rg)
{
    return dot(rg, vec2(1.0, 1.0 / 255.0));
}

vec2 packFloatToRG16(float v)
{
    v = clamp(v, 0.0, 1.0);
    vec2 enc = vec2(1.0, 65535.0) * v;
    enc = fract(enc);
    enc.x -= enc.y * (1.0 / 65535.0); // fix carry
    return (enc);
}

float unpackRG16ToFloat(vec2 rg)
{
    return dot(rg, vec2(1.0, 1.0 / 65535.0));
}

#endif