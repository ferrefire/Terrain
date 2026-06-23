#ifndef NOISE_INCLUDED
#define NOISE_INCLUDED

vec2 hash(in vec2 x)
{
	const vec2 k = vec2(0.3183099, 0.3678794);
	x = x * k + k.yx;
	return (-1.0 + 2.0*fract(16.0 * k*fract( x.x*x.y*(x.x+x.y))));
}

vec3 noised(in vec2 x)
{
	vec2 i = floor(x);
	vec2 f = fract(x);

	vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
	vec2 du = 30.0 * f * f * (f * (f - 2.0) + 1.0);
		
	vec2 ga = hash(i + vec2(0.0,0.0));
	vec2 gb = hash(i + vec2(1.0,0.0));
	vec2 gc = hash(i + vec2(0.0,1.0));
	vec2 gd = hash(i + vec2(1.0,1.0));
		
	float va = dot(ga, f - vec2(0.0,0.0));
	float vb = dot(gb, f - vec2(1.0,0.0));
	float vc = dot(gc, f - vec2(0.0,1.0));
	float vd = dot(gd, f - vec2(1.0,1.0));

	return (vec3(va + u.x * (vb - va) + u.y * (vc - va) + u.x * u.y * (va - vb - vc + vd),
				 ga + u.x * (gb - ga) + u.y * (gc - ga) + u.x * u.y * (ga - gb - gc + gd) +
				 du * (u.yx * (va - vb - vc + vd) + vec2(vb, vc) - va)));
}

struct Noise2D
{
	float val;
	vec2 grad;
	mat2 hess;
};

float _fade(float t) {return (t * t * t * (t * (t * 6.0 - 15.0) + 10.0));}
float _dfade(float t) {return (30.0 * t * t * (t - 1.0) * (t - 1.0));}
float _d2fade(float t) {return (60.0 * t * (t - 1.0) * (2.0 * t - 1.0));}

vec2 hash(vec2);

vec3 DerivativeToNormal(vec2 derivatives)
{
	float kx = 10000;
	float ky = maxHeight;
	float kz = 10000;

	vec3 result = normalize(vec3(-(ky / kx) * derivatives.x, 1.0, -(ky / kz) * derivatives.y));

	return (result);
}

float hash21(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return (fract((p3.x + p3.y) * p3.z));
}

//const float uSeed = 0.06;
//const float uSeed = 0.0425;
//const float uSeed = 0.0211256;
const float uSeed = 0.303586;
//const float uSeed = 0.12925765;
//const float uSeed = 1244662.0;

float rnd(vec2 p)
{
	vec2 salt = vec2(uSeed, uSeed * 1.6180339);
	return (hash21(p + salt));
}

float rnd(vec2 p, float seed)
{
	vec2 salt = vec2(seed, seed * 1.6180339);
	return (hash21(p + salt));
}

float fade(float t) {return (t * t * t * (t * (t * 6.0 - 15.0) + 10.0));}
float dfade(float t) {return (30.0 * t * t * (t - 1.0) * (t - 1.0));}

vec3 valueNoise2D_deriv_tiled(vec2 x, float T, float seed)
{
	vec2 pi = floor(x);
	vec2 pf = fract(x);

	vec2 p0 = pi;
	vec2 p1 = pi + 1.0;

	float a = rnd(p0, seed);
	float b = rnd(vec2(p1.x, p0.y), seed);
	float c = rnd(vec2(p0.x, p1.y), seed);
	float d = rnd(p1, seed);

	float k0 = a;
	float k1 = b - a;
	float k2 = c - a;
	float k3 = a - b - c + d;

	float ux = fade(pf.x);
	float uy = fade(pf.y);
	float dux = dfade(pf.x);
	float duy = dfade(pf.y);

	float v01 = k0 + k1 * ux + k2 * uy + k3 * ux * uy;

	float dndx01 = (k1 + k3 * uy) * dux;
	float dndy01 = (k2 + k3 * ux) * duy;

	return (vec3(v01, dndx01, dndy01));
}

vec3 fbmTest( in vec2 x, in int octaves )
{
	float f = 1.98;
	float s = 0.49;
	float a = 0.0;
	float b = 0.5;
	vec2  d = vec2(0.0, 0.0);
	mat2  m = mat2(1.0, 0.0, 
				   0.0, 1.0);
	for(int i = 0; i < octaves; i++)
	{
		vec3 n = noised(x);
		a += b * n.x;
		d += b * m * n.yz;
		b *= s;
		x = f * m * x;
		m = f * m * i * m;
	}
	return (vec3(a, d));
}

vec3 NoisePower(vec3 noise, int power)
{
	float height = pow(noise.x, power);
	float hx = power * pow(noise.x, power - 1) * noise.y;
	float hz = power * pow(noise.x, power - 1) * noise.z;

	return (vec3(height, hx, hz));
}

//const mat2 m = mat2(0.8,-0.6,0.6,0.8);
const mat2 m_hill = mat2(0.8, -0.6, 0.6, 0.8);
//const mat2 m = mat2(0.825,-0.6,0.6,0.825);
const mat2 m_steep = mat2(0.825, -0.6, 0.6, 0.825);
const mat2 m_flat = mat2(0.70, -0.5, 0.5, 0.70);
const mat2 mi = mat2(0.8, 0.6, -0.6, 0.8);

float SimpleNoise(vec2 p, float seed)
{
	return (valueNoise2D_deriv_tiled(p, 10, seed).x);
}

float SimpleFractalNoise(vec2 p, float seed, int octaves)
{
	float result = 0.0;
	float scale = 1.0;
	float strength = 1.0;
	float total = 0.0;

	for (int i = 0; i < octaves; i++)
	{
		result += valueNoise2D_deriv_tiled(p * scale, 10, seed).x * strength;
		total += strength;

		strength *= 0.5;
		scale *= 2.0;
	}

	result /= total;

	return (result);
}

float terrain(vec2 p, int octaves, float erosion, mat2 m, float seed)
{
	float a = 0.0;
	float b = 1.0;
	vec2 d = vec2(0.0, 0.0);
	float total = 0.0;
	for(int i = 0; i < octaves; i++)
	{
		vec3 n = valueNoise2D_deriv_tiled(p, 10, seed);
		//vec3 n = NoisePower(valueNoise2D_deriv_tiled(p, 10), 2);
		d += n.yz;
		a += b * n.x / (1.0 + dot(d, d) * erosion);
		total += b;
		b *= 0.5;
		p = m * p * 2.0;
	}
	return (a / total);
	//return a / (total / 1.25);
	//return a;
}

float HeightPower(float height, float power)
{
	//height = height * 2.0 - 1.0;
	//float heightSign = sign(height);
	//float result = clamp(pow(abs(height), power) * heightSign, 0.0, 1.0);
	//result *= heightSign;

	return (pow(height, power));
}

vec3 TerrainData(vec2 uv, int octaves, float sampleDis, bool heightOnly, float seed, float erodeFactor, float steepness)
{
	octaves = clamp(octaves, 1, 20);
	//float biome = 1;
	//float noise = terrainBiome(uv, octaves, biome);
	float noise = terrain(uv, octaves, erodeFactor, m_steep, seed);

	float height = HeightPower(noise, steepness);
	vec2 derivatives = vec2(0.0);

	if (!heightOnly)
	{
		//float dis = 0.0001;

		float dis = sampleDis;

		float noiserx = 0.0;
		float noiseuy = 0.0;

		noiserx = HeightPower(terrain(uv + (vec2(dis, 0)), octaves, erodeFactor, m_steep, seed), steepness);
		noiseuy = HeightPower(terrain(uv + (vec2(0, dis)), octaves, erodeFactor, m_steep, seed), steepness);

		derivatives = vec2((noiserx - height) / (dis), (noiseuy - height) / (dis));
	}

	return (vec3(height, derivatives));
}

#endif