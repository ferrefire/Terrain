#ifndef ATMOSPHERE_INCLUDED
#define ATMOSPHERE_INCLUDED

layout(set = 0, binding = 5) uniform AtmosphereData
{
	float miePhaseFunction;
	float offsetRadius;
	float rayleighScatteringStrength;
	float mieScatteringStrength;
	float mieExtinctionStrength;
	float absorptionExtinctionStrength;
	float mistStrength;
	float skyStrength;
	float rayleighScaleHeight;
	float mieScaleHeight;
	float cameraScale;
	float absorptionDensityHeight;
	float absorption1;
	float absorption2;
	float absorption3;
	float absorption4;
} atmosphereData;

#define PI 3.1415926535897932384626433832795

const float bottomRadius = 6360.0;
const float topRadius = 6460.0;
//const float offsetRadius = 0.01; // Maybe change back to 0.0001
//const float rayleighScaleHeight = 8.0;
//const float mieScaleHeight = 1.2;

//const float[10] rayleighDensity = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, -1.0 / rayleighScaleHeight, 0.0, 0.0};
//const float[10] mieDensity = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, -1.0 / mieScaleHeight, 0.0, 0.0};
//const float[10] absorptionDensity = {25.0, 0.0, 0.0, 1.0 / 15.0, -2.0 / 3.0, 0.0, 0.0, 0.0, -1.0 / 15.0, 8.0 / 3.0};

const vec3 rayleighScattering = vec3(0.005802, 0.013558, 0.033100);
const vec3 mieScattering = vec3(0.003996, 0.003996, 0.003996);

const vec3 mieExtinction = vec3(0.004440, 0.004440, 0.004440);
const vec3 absorptionExtinction = vec3(0.000650, 0.001881, 0.000085);

//const float miePhaseFunction = 0.8;

const float bottomRadius2 = bottomRadius * bottomRadius;
const float topRadius2 = topRadius * topRadius;
const float H = sqrt(topRadius2 - bottomRadius2);

const vec2 transmittanceDimensions = vec2(256, 64);
const vec2 scatteringDimensions = vec2(32, 32);
const vec2 skyDimensions = vec2(192, 128);
const vec3 aerialDimensions = vec3(64, 64, 32);

//const float cameraScale = 0.001;

struct ScatteringResult
{
	vec3 ray;
	vec3 mie;
};

float SubUVToUnit(float u, float resolution)
{
	return ((u - 0.5 / resolution) * (resolution / (resolution - 1.0)));
}

float UnitToSubUV(float u, float resolution)
{
	return ((u + 0.5f / resolution) * (resolution / (resolution + 1.0)));
}

vec2 UVToTransmittance(vec2 uv) // Maybe use a safe sqrt instead!
{
	vec2 result = vec2(0.0, 1.0);

	//float H = sqrt(topRadius2 - bottomRadius2);

	float rho = H * uv.y;
	float rho2 = rho * rho;
	result.x = sqrt(rho2 + bottomRadius2);

	float dMin = topRadius - result.x;
	float dMax = rho + H;
	float d = dMin + uv.x * (dMax - dMin);

	if (d != 0.0) {result.y = ((H * H) - rho2 - (d * d)) / (2.0 * result.x * d);}

	result.y = clamp(result.y, -1.0, 1.0);

	return (result);
}

vec2 TransmittanceToUV(vec2 uv) // Maybe use a safe sqrt instead!
{
	//float H = sqrt(topRadius2 - bottomRadius2);
	float uvx2 = uv.x * uv.x;
	float uvy2 = uv.y * uv.y;

	float rho = sqrt(uvx2 - bottomRadius2);
	float det = uvx2 * (uvy2 - 1.0) + topRadius2;

	float d = max(0.0, (-uv.x * uv.y + sqrt(det)));
	float dMin = topRadius - uv.x;
	float dMax = rho + H;
	float mu = (d - dMin) / (dMax - dMin);
	float r = rho / H;

	return (vec2(mu, r));
}

vec2 UVToSky(vec2 uv, float height)
{
	uv = vec2(SubUVToUnit(uv.x, skyDimensions.x), SubUVToUnit(uv.y, skyDimensions.y));

	float beta = asin(bottomRadius / height);
	float horizonAngle = PI - beta;

	float viewAngle = 0;
	float lightAngle = 0;

	if (uv.y < 0.5)
	{
		float coord = 1.0 - (1.0 - 2.0 * uv.y) * (1.0 - 2.0 * uv.y);
		viewAngle = horizonAngle * coord;
	}
	else
	{
		float coord = (uv.y * 2.0 - 1.0) * (uv.y * 2.0 - 1.0);
		viewAngle = horizonAngle + beta * coord;
	}

	lightAngle = (uv.x * uv.x) * PI;

	return (vec2(viewAngle, lightAngle));
}

vec2 SkyToUV(bool groundIntersect, vec2 uvS, float viewHeight)
{
	vec2 uv = vec2(0.0);

	float beta = asin(bottomRadius / viewHeight);
	float horizonAngle = PI - beta;

	if (!groundIntersect)
	{
		float coord = uvS.x / horizonAngle;
		coord = (1.0 - sqrt(1.0 - coord)) / 2.0;
		uv.y = coord;
	}
	else
	{
		float coord = (uvS.x - horizonAngle) / beta;
		coord = (sqrt(coord) + 1.0) / 2.0;
		uv.y = coord;
	}

	uv.x = sqrt(uvS.y / PI);

	uv = vec2(UnitToSubUV(uv.x, skyDimensions.x), UnitToSubUV(uv.y, skyDimensions.y));

	return (uv);
}

float IntersectSphere(vec3 position, vec3 direction, vec3 center, float radius)
{
	float a = dot(direction, direction);
	vec3 centerDirection = position - center;
	float b = 2.0 * dot(direction, centerDirection);
	float c = dot(centerDirection, centerDirection) - (radius * radius);
	float delta = (b * b) - (4.0 * a * c);

	if (delta < 0.0 || a == 0.0) {return (-1.0);}

	float deltaSqrt = sqrt(delta);

	float hit1 = (-b - deltaSqrt) / (2.0 * a);
	float hit2 = (-b + deltaSqrt) / (2.0 * a);

	if (hit1 < 0.0 && hit2 < 0.0) {return (-1.0);}

	if (hit1 < 0.0) {return (max(0.0, hit2));}
	else if (hit2 < 0.0) {return (max(0.0, hit1));}

	return (max(0.0, min(hit1, hit2)));
}

vec3 SampleExtinction(vec3 worldPosition)
{
	const float viewHeight = length(worldPosition) - bottomRadius;
	const float rayleighDensity = -1.0 / atmosphereData.rayleighScaleHeight;
	const float mieDensity = -1.0 / atmosphereData.mieScaleHeight;

	const float densityRay = exp(rayleighDensity * viewHeight);
	const float densityMie = exp(mieDensity * viewHeight);
	const float densityOzo = clamp(viewHeight < atmosphereData.absorptionDensityHeight ? 
		atmosphereData.absorption1 * viewHeight + atmosphereData.absorption2 : 
		atmosphereData.absorption3 * viewHeight + atmosphereData.absorption4, 
		0.0, 1.0);

	vec3 extinctionRay = (rayleighScattering * atmosphereData.rayleighScatteringStrength) * densityRay;
	vec3 extinctionMie = (mieExtinction * atmosphereData.mieExtinctionStrength) * densityMie;
	vec3 extinctionOzo = (absorptionExtinction * atmosphereData.absorptionExtinctionStrength) * densityOzo;

	return (extinctionRay + extinctionMie + extinctionOzo);
}

ScatteringResult SampleScatteringResult(vec3 worldPosition)
{
	const float viewHeight = length(worldPosition) - bottomRadius;
	const float rayleighDensity = -1.0 / atmosphereData.rayleighScaleHeight;
	const float mieDensity = -1.0 / atmosphereData.mieScaleHeight;

	const float densityRay = exp(rayleighDensity * viewHeight);
	const float densityMie = exp(mieDensity * viewHeight);
	const float densityOzo = clamp(viewHeight < atmosphereData.absorptionDensityHeight ? 
		atmosphereData.absorption1 * viewHeight + atmosphereData.absorption2 : 
		atmosphereData.absorption3 * viewHeight + atmosphereData.absorption4, 
		0.0, 1.0);

	ScatteringResult result = ScatteringResult(vec3(0.0), vec3(0.0));

	//vec3 scatteringRay = rayleighScattering * densityRay;
	//vec3 scatteringMie = mieScattering * densityMie;
	//vec3 scatteringOzo = vec3(0.0);
	//return (scatteringRay + scatteringMie + scatteringOzo);

	result.ray = (rayleighScattering * atmosphereData.rayleighScatteringStrength) * densityRay;
	result.mie = (mieScattering * atmosphereData.mieScatteringStrength) * densityMie;

	return (result);
}

vec3 SampleScattering(vec3 worldPosition)
{
	ScatteringResult result = SampleScatteringResult(worldPosition);

	return (result.ray + result.mie);
}

bool MoveToAtmosphere(inout vec3 worldPosition, vec3 worldDirection)
{
	if (length(worldPosition) > topRadius)
	{
		float atmosphereDistance = IntersectSphere(worldPosition, worldDirection, vec3(0.0), topRadius);

		if (atmosphereDistance == -1.0)
		{
			return (false);
		}
		else
		{
			vec3 offset = normalize(worldPosition) * -atmosphereData.offsetRadius;
			worldPosition += worldDirection * atmosphereDistance + offset;
		}
	}

	return (true);
}

float MiePhase(float g, float theta)
{
    float k = 3.0 / (8.0 * PI) * (1.0 - g * g) / (2.0 + g * g);
    return (k * (1.0 + theta * theta) / pow(1.0 + g * g - 2.0 * g * -theta, 1.5));
}

float RayleighPhase(float theta)
{
    float factor = 3.0 / (16.0 * PI);
    return (factor * (1.0 + theta * theta));
}

#endif