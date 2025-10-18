#ifndef NOISE_INCLUDED
#define NOISE_INCLUDED

vec2 hash( in vec2 x )
{
    const vec2 k = vec2( 0.3183099, 0.3678794 );
    x = x*k + k.yx;
    return -1.0 + 2.0*fract( 16.0 * k*fract( x.x*x.y*(x.x+x.y)) );
}

vec3 noised( in vec2 x )
{
    vec2 i = floor( x );
    vec2 f = fract( x );

    vec2 u = f*f*f*(f*(f*6.0-15.0)+10.0);
    vec2 du = 30.0*f*f*(f*(f-2.0)+1.0);
    
    vec2 ga = hash( i + vec2(0.0,0.0) );
    vec2 gb = hash( i + vec2(1.0,0.0) );
    vec2 gc = hash( i + vec2(0.0,1.0) );
    vec2 gd = hash( i + vec2(1.0,1.0) );
    
    float va = dot( ga, f - vec2(0.0,0.0) );
    float vb = dot( gb, f - vec2(1.0,0.0) );
    float vc = dot( gc, f - vec2(0.0,1.0) );
    float vd = dot( gd, f - vec2(1.0,1.0) );

    return vec3( va + u.x*(vb-va) + u.y*(vc-va) + u.x*u.y*(va-vb-vc+vd),   // value
                 ga + u.x*(gb-ga) + u.y*(gc-ga) + u.x*u.y*(ga-gb-gc+gd) +  // derivatives
                 du * (u.yx*(va-vb-vc+vd) + vec2(vb,vc) - va));
}

vec3 DerivativeToNormal(vec2 derivatives)
{
	float kx = 10000;
	float ky = 5000;
	float kz = 10000;

	vec3 result = normalize(vec3(-(ky/kx) * derivatives.x, 1.0, -(ky/kz) * derivatives.y));

	return (result);
}

// Hash from integer lattice -> [0,1). Expects integer-like inputs.
float hash21(vec2 p)
{
    // Any good integer hash is fine; this one works on float "ints".
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Quintic fade and derivative (correct)
float fade(float t)  { return t*t*t*(t*(t*6.0 - 15.0) + 10.0); }
float dfade(float t) { return 30.0*t*t*(t - 1.0)*(t - 1.0); }  // 30 t^2 (t-1)^2

// 2D value noise in [−1,1] with analytic d/dx, d/dy, tile period T (integer)
vec3 valueNoise2D_deriv_tiled(vec2 x, float T)
{
    vec2 pi = floor(x);
    vec2 pf = fract(x);

    // wrap base and neighbor indices
    vec2 p0 = mod(pi,       T);
    vec2 p1 = mod(pi + 1.0, T);

    // four corners (wrapped)
    float a = hash21(p0);
    float b = hash21(vec2(p1.x, p0.y));
    float c = hash21(vec2(p0.x, p1.y));
    float d = hash21(p1);

    // bilinear polynomial coefficients
    float k0 = a;
    float k1 = b - a;
    float k2 = c - a;
    float k3 = a - b - c + d;

    // fade and derivatives
    float ux = fade(pf.x);
    float uy = fade(pf.y);
    float dux = dfade(pf.x);
    float duy = dfade(pf.y);

    // value in [0,1]
    float v01 = k0 + k1*ux + k2*uy + k3*ux*uy;
    // map to [-1,1]
    //float v   = -1.0 + 2.0*v01;

    // derivatives of the [0,1] value
    float dndx01 = (k1 + k3*uy) * dux;
    float dndy01 = (k2 + k3*ux) * duy;

    // scale derivatives by 2 to match the [-1,1] remap
    return vec3(v01, dndx01, dndy01);
    //return vec3(v, 2.0*dndx01, 2.0*dndy01);
}

// Returns fBm value and gradient (d/dx, d/dy)
vec3 fbm2D_withDeriv(vec2 x, int octaves, float lacunarity, float gain, bool erode)
{
    float amp = 1.0;
    //float amp = 0.75008;
    //float freq = 2.5;
    float freq = 1.5;
	float div = 0.0;
	vec2 erosionVector = vec2(0.0);
	float erodeScale = 1.0;

    float v = 0.0;
    vec2  g = vec2(0.0);

    for (int i = 0; i < octaves; ++i)
	{
        vec3 nd = valueNoise2D_deriv_tiled(x * freq, 10000);

		erosionVector += nd.yz * erodeScale;
		//float steepness = 1.0 / (1.0 + length(erosionVector) * 1.5);
		float steepness = 1.0 / (1.0 + length(nd.yz) * 0.1);
		//steepness = 1;
		v += amp * nd.x;
        g += amp * nd.yz * freq; // chain rule
		div += amp;
        //v += amp * nd.x / (erode ? steepness : 1.0);
        //g += amp * nd.yz * freq / (erode ? steepness : 1.0); // chain rule
		//div += amp / (erode ? steepness : 1.0);
        freq *= lacunarity;
        amp *= gain;
		erodeScale *= 0.85;
        //amp = pow(gain, i + 1) * steepness;
    }

	//v /= div;
	//g /= div;

    return vec3(v, g) / div;
}

vec3 fbm(in vec2 uv, int octaves, float lacunarity, float gain)
{
	float w = 2.5;
	//float w = 1.5;
	float div = 0.0;
	float scale = 1.0;
	float strength = 1.0;
	vec3 result = vec3(0);

	for (int i = 0; i < octaves; i++)
	{
		vec3 noise = noised(uv * (w * scale));
		noise.yz *= (w * scale);
		result += noise * strength;
		div += strength;

		scale *= lacunarity;
		strength *= gain;
	}

	result /= div;

	return (result);
}

vec3 TerrainHeight(vec2 uv, bool erode)
{
	vec3 noise = fbm2D_withDeriv(uv + 17, 6, 4, 0.2, erode);
	//vec3 noise = fbm(uv, 5, 2, 0.5);

	const int power = 4;
	//float height = pow(noise.x, power);
	//float hx = power * pow(noise.x, power - 1) * noise.y;
	//float hz = power * pow(noise.x, power - 1) * noise.z;

	float height = exp(-power * noise.x);
	float hx = -power * exp(-power * noise.x) * noise.y;
	float hz = -power * exp(-power * noise.x) * noise.z;

	return (vec3(height, hx, hz));
}

#endif