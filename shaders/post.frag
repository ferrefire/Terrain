#version 460

#extension GL_GOOGLE_include_directive : require

#include "atmosphere.glsl"

#include "variables.glsl"

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput gAlbedo;

layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput gDepth;

//layout(set = 0, binding = 0) uniform sampler2D screenTexture;
//layout(set = 0, binding = 1) uniform sampler2D transmittanceTexture;
//layout(set = 0, binding = 2) uniform sampler2D scatteringTexture;
//layout(set = 0, binding = 3) uniform sampler2D skyTexture;
//layout(set = 0, binding = 4, std430) buffer LuminanceData{float value;} luminanceData;
layout(set = 1, binding = 2) uniform sampler2D transmittanceTexture;
layout(set = 1, binding = 3) uniform sampler2D scatteringTexture;
layout(set = 1, binding = 4) uniform sampler2D skyTexture;
layout(set = 1, binding = 5) uniform sampler3D aerialTexture;
layout(set = 1, binding = 6, std430) buffer LuminanceData{float value;} luminanceData;

layout(location = 0) in vec2 worldCoordinates;

layout(location = 0) out vec4 pixelColor;

const float exposure = 1;

vec3 Reihard(vec3 base)
{
	return (base / (1.0 + base));
}

vec3 Hable(vec3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    float W = 11.2; // white point

    vec3 num   = ((x * (A * x + C * B)) + D * E);
    vec3 denom = (x * (A * x + B) + D * F);
    vec3 color = num / denom - E / F;

    // normalize so that white point W maps to 1.0
    float white = (((W * (A * W + C * B)) + D * E) /
                   ((W * (A * W + B) + D * F)) - E / F);
    return color / white;
}

vec3 RRTAndODTFit(vec3 v) {
    vec3 a = v * (v + 0.0245786) - 0.000090537;
    vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
    return a / b;
}

vec3 acesTonemap(vec3 color) {
    // transform from sRGB linear to “ACES” like space
    //const mat3 ACESInputMat = mat3(
    //     0.59719, 0.35458, 0.04823,
    //     0.07600, 0.90834, 0.01566,
    //     0.02840, 0.13383, 0.83777
    //);

	const mat3 ACESInputMat = mat3(
         0.59719, 0.07600, 0.02840,
         0.35458, 0.90834, 0.13383,
         0.04823, 0.01566, 0.83777
    );

    //const mat3 ACESOutputMat = mat3(
    //     1.60475, -0.53108, -0.07367,
    //    -0.10208,  1.10813, -0.00605,
    //    -0.00327, -0.07276,  1.07602
    //);

	const mat3 ACESOutputMat = mat3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );

    color = ACESInputMat * color;
    color = RRTAndODTFit(color);
    color = ACESOutputMat * color;
    return clamp(color, 0.0, 1.0);
}

vec3 sunWithBloom(vec3 worldDir, vec3 sunDir)
{
    const float sunSolidAngle = 1.0 * PI / 180.0;
    const float minSunCosTheta = cos(sunSolidAngle);

    float cosTheta = dot(worldDir, sunDir);
    if(cosTheta >= minSunCosTheta) {return vec3(0.5) ;}
    float offset = minSunCosTheta - cosTheta;
    float gaussianBloom = exp(-offset * 50000.0) * 0.5;
    float invBloom = 1.0/(0.02 + offset * 300.0) * 0.01;
    return vec3(gaussianBloom + invBloom);
}

vec4 GetAerial(float depth, mat4 invViewProjMat)
{
	vec3 clipSpace = vec3(worldCoordinates * vec2(2.0) - vec2(1.0), depth);
	vec4 hPos = invViewProjMat * vec4(clipSpace, 1.0);
	vec3 cameraRayWorld = normalize(hPos.xyz / hPos.w - variables.viewPosition.xyz);
	float realDepth = length(hPos.xyz / hPos.w - variables.viewPosition.xyz);

	float slice = realDepth * cameraScale * (1.0 / 4.0); // Maybe use 0.1 as cameraScale
    //float weight = 1.0;

	//if (slice < 0.5)
	//{
	//	weight = clamp(slice * 2.0, 0.0, 1.0);
    //    slice = 0.5;
	//}

	const float aerialTexelSize = 1.0 / float(aerialDimensions.x);
	const float offset = aerialTexelSize * 0.5;

	float w = sqrt(slice / aerialDimensions.z);
    //vec4 aerialValue = texture(aerialTexture, vec3(worldCoordinates, w));
    vec4 aerialValue = texture(aerialTexture, vec3(worldCoordinates + vec2(-offset), w));
    aerialValue += texture(aerialTexture, vec3(worldCoordinates + vec2(offset, -offset), w));
    aerialValue += texture(aerialTexture, vec3(worldCoordinates + vec2(-offset, offset), w));
    aerialValue += texture(aerialTexture, vec3(worldCoordinates + vec2(offset), w));
	aerialValue *= 0.25;
    //vec4 aerialValue = weight * texture(aerialTexture, vec3(worldCoordinates, w));

	//if (slice - floor(slice) > 0.9)
	//{
	//	w = sqrt(floor(slice + 1.0) / aerialDimensions.z);
	//	aerialValue.rgb += texture(aerialTexture, vec3(worldCoordinates, w)).rgb;
	//	aerialValue.rgb *= 0.5;
	//	aerialValue.rgb = vec3(0, 0, 1);
	//}

	//if (w > 0.4) return (vec4(100));

	return (aerialValue);
}

void main()
{
	//vec3 color = texture(screenTexture, worldCoordinates).rgb;

	vec3 color = vec3(0.0);
	float depth = subpassLoad(gDepth).x;

	//if (depth >= 1.0) {color = vec3(0.0);}

	//float currentExposure = 1.0 / (averageLuminance + 0.5);
	//float currentExposure = 1.0 / (averageLuminance);
	//currentExposure = pow(currentExposure, 0.25);

	//vec3 sunDirection = normalize(vec3(0.39036, 0.48795, -0.78072));
	mat4 invViewProjMat = inverse(variables.projection * variables.view);
    vec2 pixPos = worldCoordinates;
    vec3 clipSpace = vec3(pixPos * vec2(2.0) - vec2(1.0), 1.0);
    
    vec4 Hpos = invViewProjMat * vec4(clipSpace, 1.0);

	vec3 viewPosition = (variables.viewPosition.xyz + vec3(0.0, 2500.0 + (variables.terrainOffset.y * 10000.0), 0.0)) * cameraScale;

    vec3 worldDirection = normalize(Hpos.xyz / Hpos.w - variables.viewPosition.xyz);
	vec3 worldPosistion = viewPosition + vec3(0.0, bottomRadius, 0.0);

	float viewHeight = length(worldPosistion);

	vec4 aerialResult = GetAerial(depth, invViewProjMat);
	//aerialResult.w = pow(aerialResult.w, 2.0);
	//vec3 mistColor = aerialResult.rgb * (1.0 - aerialResult.w);
	//vec3 mistColor = aerialResult.rgb * pow(aerialResult.w, 0.125 * 0.5);
	vec3 mistColor = aerialResult.rgb;
	//vec3 mistColor = aerialResult.rgb * aerialResult.w * 64;
	//vec3 mistColor = aerialResult.rgb * aerialResult.w * 48;

	if (depth >= 1.0)
	{
		vec2 uv = vec2(0.0);
		vec3 up = normalize(worldPosistion);
		float viewAngle = acos(dot(worldDirection, up));

		float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
		bool groundIntersect = IntersectSphere(worldPosistion, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

		uv = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

		color = texture(skyTexture, uv).rgb * 12;

		//color += mistColor * 16;

		if (!groundIntersect) {color += sunWithBloom(worldDirection, variables.lightDirection.xyz) * vec3(1.0, 0.9, 0.7) * 24;}
	}
	else
	{
		color = subpassLoad(gAlbedo).rgb;

		//vec4 aerialResult = GetAerial(depth, invViewProjMat);

		//color = mix(color, aerialResult.rgb * 24.0, clamp(1.0 - pow(aerialResult.w, 64.0) - 0.25, 0.0, 1.0));
		//color += aerialResult.rgb * 16.0 * aerialResult.w;
		//color = vec3(aerialResult.w);

		//color += mistColor * 48;
	}

	//color = color * (aerialResult.w) + mistColor * 64;
	color += mistColor * 48;

	//int sliceX = int(floor(worldCoordinates.x * 8));
	//int sliceY = int(floor(worldCoordinates.y * 4));
	//int slice = sliceX + sliceY * 8;
	//float coordX = worldCoordinates.x * 8 - sliceX;
	//float coordY = worldCoordinates.y * 4 - sliceY;
	//color = texture(aerialTexture, vec3(coordX, coordY, float(slice) / 32.0)).rgb * 3.0;
	//color = texture(skyTexture, worldCoordinates).rgb * 6.0;

	//vec3 exposedColor = color * currentExposure;
	//vec3 exposedColor = color * pow(1.0 / luminanceData.value, 0.25);
	vec3 exposedColor = color * 1.0;
	//vec3 exposedColor = color * 1.25;

	//vec3 mappedColor = acesTonemap(texture(skyTexture, worldCoordinates).rgb * 6.0);

	//exposedColor = vec3(texture(glillTexture, worldCoordinates).r);

	vec3 mappedColor = acesTonemap(exposedColor);
	
	pixelColor = vec4(mappedColor, 1.0);
	//pixelColor = vec4(texture(skyTexture, worldCoordinates).rgb, 1.0);
}