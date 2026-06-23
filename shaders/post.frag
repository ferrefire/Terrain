#version 460

#extension GL_GOOGLE_include_directive : require

#include "atmosphere.glsl"

#include "variables.glsl"
#include "transformation.glsl"

layout(input_attachment_index = 0, set = 1, binding = 0) uniform subpassInput gAlbedo;

layout(input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput gDepth;

layout(set = 1, binding = 2) uniform sampler2D transmittanceTexture;
layout(set = 1, binding = 3) uniform sampler2D scatteringTexture;
layout(set = 1, binding = 4) uniform sampler2D skyTexture;
layout(set = 1, binding = 5) uniform sampler3D aerialTexture;
layout(set = 1, binding = 6, std430) buffer LuminanceData{float value;} luminanceData;

layout(set = 1, binding = 7, std140) uniform PostData
{
	uint useLinearDepth;
	uint aerialBlendMode;
	float aerialBlendDistance;
	uint toneMapping;
	float exposure;
} postData;

layout(set = 1, binding = 8) uniform sampler2DShadow screenTexture;


layout(location = 0) in vec2 worldCoordinates;

layout(location = 0) out vec4 pixelColor;

const float exposure = 1;

vec3 Reihard(vec3 base)
{
	return (base / (1.0 + base));
}

vec3 Hable(vec3 x)
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	float W = 11.2;

	vec3 num   = ((x * (A * x + C * B)) + D * E);
	vec3 denom = (x * (A * x + B) + D * F);
	vec3 color = num / denom - E / F;

	float white = (((W * (A * W + C * B)) + D * E) /
				   ((W * (A * W + B) + D * F)) - E / F);

	return (color / white);
}

vec3 RRTAndODTFit(vec3 v)
{
	vec3 a = v * (v + 0.0245786) - 0.000090537;
	vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081;
	return (a / b);
}

vec3 acesTonemap(vec3 color)
{

	const mat3 ACESInputMat = mat3(
		 0.59719, 0.07600, 0.02840,
		 0.35458, 0.90834, 0.13383,
		 0.04823, 0.01566, 0.83777
	);

	//const mat3 ACESOutputMat = mat3(
	//	 1.60475, -0.53108, -0.07367,
	//	-0.10208,  1.10813, -0.00605,
	//	-0.00327, -0.07276,  1.07602
	//);

	const mat3 ACESOutputMat = mat3(
		 1.60475, -0.10208, -0.00327,
		-0.53108,  1.10813, -0.07276,
		-0.07367, -0.00605,  1.07602
	);

	color = ACESInputMat * color;
	color = RRTAndODTFit(color);
	color = ACESOutputMat * color;

	return (clamp(color, 0.0, 1.0));
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

	return (vec3(gaussianBloom + invBloom));
}

vec4 GetAerial(float depth, vec3 pixelWorldPos)
{
	float realDepth = length(pixelWorldPos - variables.viewPosition.xyz);

	float slice = realDepth * atmosphereData.cameraScale / atmosphereData.aerialSliceScale;
	float weight = 1.0;

	//if (slice < 0.5)
	//{
	//	weight = clamp(slice * 2.0, 0.0, 1.0);
	//	slice = 0.5;
	//}

	const vec2 aerialTexelSize = vec2(1.0) / vec2(atmosphereData.aerialDimensions.xy);
	const vec2 offset = aerialTexelSize * 0.5;

	//float w = sqrt(slice / atmosphereData.aerialDimensions.z);
	float w = slice / atmosphereData.aerialDimensions.z;

	for (int i = 0; i < atmosphereData.aerialSlicePower; i++) {w = sqrt(w);}

	w = clamp(w, 0.0, 1.0);

	vec4 aerialValue = vec4(0.0);

	if (postData.aerialBlendMode == 0 || (postData.aerialBlendMode == 3 && w > postData.aerialBlendDistance))
	{
		aerialValue = texture(aerialTexture, vec3(worldCoordinates, w));
	}
	else if (postData.aerialBlendMode == 1 || (postData.aerialBlendMode == 3 && w <= postData.aerialBlendDistance))
	{
		aerialValue = texture(aerialTexture, vec3(worldCoordinates + -offset, w));
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + (offset * vec2(1.0, -1.0)), w));
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + (offset * vec2(-1.0, 1.0)), w));
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + offset, w));
		aerialValue *= 0.25;
	}
	else if (postData.aerialBlendMode == 2)
	{
		aerialValue = texture(aerialTexture, vec3(worldCoordinates, w)) * 0.4;
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + -offset * 2.0, w)) * 0.15;
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + (offset * 2.0 * vec2(1.0, -1.0)), w)) * 0.15;
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + (offset * 2.0 * vec2(-1.0, 1.0)), w)) * 0.15;
		aerialValue += texture(aerialTexture, vec3(worldCoordinates + offset * 2.0, w)) * 0.15;
	}

	return (aerialValue);
}

void main()
{
	//vec3 color = texture(screenTexture, worldCoordinates).rgb;

	vec3 color = vec3(0.0);
	float depth = subpassLoad(gDepth).x;

	mat4 invViewProjMat = inverse(variables.projection * variables.view);
	vec2 pixPos = worldCoordinates;
	vec3 clipSpace = vec3(pixPos * vec2(2.0) - vec2(1.0), depth);
		
	vec4 Hpos = invViewProjMat * vec4(clipSpace, 1.0);
	vec3 pixelWorldPos = Hpos.xyz / Hpos.w;

	vec3 viewPosition = (variables.viewPosition.xyz + vec3(0.0, maxHeight * 0.5 + (variables.terrainOffset.y * terrainDiv), 0.0)) * atmosphereData.cameraScale;

	vec3 worldDirection = normalize(pixelWorldPos - variables.viewPosition.xyz);
	vec3 worldPosistion = viewPosition + vec3(0.0, bottomRadius, 0.0);

	float viewHeight = length(worldPosistion);

	vec4 aerialResult = GetAerial(depth, pixelWorldPos);

	vec3 mistColor = aerialResult.rgb;

	if (depth >= 1.0)
	{
		vec2 uv = vec2(0.0);
		vec3 up = normalize(worldPosistion);
		float viewAngle = acos(dot(worldDirection, up));

		float lightAngle = acos(dot(normalize(vec3(variables.lightDirection.x, 0.0, variables.lightDirection.z)), normalize(vec3(worldDirection.x, 0.0, worldDirection.z))));
		bool groundIntersect = IntersectSphere(worldPosistion, worldDirection, vec3(0.0), bottomRadius) >= 0.0;

		uv = SkyToUV(groundIntersect, vec2(viewAngle, lightAngle), viewHeight);

		//color = texture(skyTexture, uv).rgb * 12;
		color = texture(skyTexture, uv).rgb * atmosphereData.skyStrength;

		//color += mistColor * 16;

		if (!groundIntersect) {color += sunWithBloom(worldDirection, variables.lightDirection.xyz) * vec3(1.0, 0.9, 0.7) * 24;}
	}
	else
	{
		color = subpassLoad(gAlbedo).rgb;
	}

	//color = color * (aerialResult.w) + mistColor * 64;
	color += mistColor * atmosphereData.mistStrength;

	//vec3 exposedColor = color * currentExposure;
	//vec3 exposedColor = color * pow(1.0 / luminanceData.value, 0.25);
	vec3 exposedColor = color * postData.exposure;
	//vec3 exposedColor = color * 1.25;

	//vec3 mappedColor = acesTonemap(texture(skyTexture, worldCoordinates).rgb * 6.0);

	//exposedColor = vec3(texture(glillTexture, worldCoordinates).r);

	vec3 mappedColor = RRTAndODTFit(exposedColor);
	if (postData.toneMapping == 1) {mappedColor = acesTonemap(exposedColor);}
	
	pixelColor = vec4(mappedColor, 1.0);
	//if (postData.useLinearDepth == 1) {pixelColor = vec4(vec3(texture(screenTexture, vec3(worldCoordinates, 1.0)).r), 1.0);}
	//pixelColor = vec4(vec3(test), 1.0);
	//pixelColor = vec4(texture(skyTexture, worldCoordinates).rgb, 1.0);
}