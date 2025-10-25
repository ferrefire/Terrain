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

struct Noise2D {
    float val;   // S
    vec2  grad;  // (Sx, Sy)
    mat2  hess;  // [ [Sxx, Sxy], [Syx, Syy] ]
};

float _fade(float t)  { return t*t*t*(t*(t*6.0 - 15.0) + 10.0); }
float _dfade(float t) { return 30.0*t*t*(t - 1.0)*(t - 1.0); }     // 30 t^2 (t-1)^2
float _d2fade(float t){ return 60.0*t*(t - 1.0)*(2.0*t - 1.0); }   // 60 t (t-1) (2t-1)

// hash() must return a unit-ish gradient direction per corner (your existing one)
vec2 hash(vec2);

Noise2D perlin2D_withHessian(in vec2 x)
{
    vec2 i = floor(x);
    vec2 f = fract(x);

    vec2 u   = vec2(_fade(f.x),  _fade(f.y));
    vec2 du  = vec2(_dfade(f.x), _dfade(f.y));
    vec2 d2u = vec2(_d2fade(f.x), _d2fade(f.y));

    // corner gradients
    vec2 g00 = hash(i + vec2(0.0,0.0));
    vec2 g10 = hash(i + vec2(1.0,0.0));
    vec2 g01 = hash(i + vec2(0.0,1.0));
    vec2 g11 = hash(i + vec2(1.0,1.0));

    // corner dot products
    float v00 = dot(g00, f - vec2(0.0,0.0));
    float v10 = dot(g10, f - vec2(1.0,0.0));
    float v01 = dot(g01, f - vec2(0.0,1.0));
    float v11 = dot(g11, f - vec2(1.0,1.0));

    // combos
    vec2  m = g00 - g10 - g01 + g11;   // vector
    float n = v00 - v10 - v01 + v11;   // scalar

    // value
    float S = v00
            + u.x*(v10 - v00)
            + u.y*(v01 - v00)
            + u.x*u.y*(v00 - v10 - v01 + v11);

    // W for the du term
    vec2 W = vec2(u.y*n + (v10 - v00),
                  u.x*n + (v01 - v00));

    // gradient: D = Gblend + du * W
    vec2 Gblend = g00
                + u.x*(g10 - g00)
                + u.y*(g01 - g00)
                + u.x*u.y*m;
    vec2 D = Gblend + du * W;

    // partials of Gblend
    vec2 dGdx = du.x * ( (g10 - g00) + u.y * m );
    vec2 dGdy = du.y * ( (g01 - g00) + u.x * m );

    // partials of W
    float dWx_dx = u.y * m.x + (g10.x - g00.x);
    float dWx_dy = du.y * n   + u.y * m.y + (g10.y - g00.y);

    float dWy_dx = du.x * n   + u.x * m.x + (g01.x - g00.x);
    float dWy_dy =              u.x * m.y + (g01.y - g00.y);

    // Hessian entries
    float Sxx = dGdx.x + d2u.x * W.x + du.x * dWx_dx;
    float Sxy = dGdy.x +              du.x * dWx_dy;
    float Syx = dGdx.y +              du.y * dWy_dx;
    float Syy = dGdy.y + d2u.y * W.y + du.y * dWy_dy;

    Noise2D outp;
    outp.val  = S;
    outp.grad = D;
    outp.hess = mat2(Sxx, Sxy,
                     Syx, Syy);
    return outp;
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
vec3 valueNoiseFbm(vec2 x, int octaves, float lacunarity, float gain, bool erode)
{
    float amp = 1.0;
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
		v += amp * nd.x;
        g += amp * nd.yz * freq; // chain rule
		div += amp;

        freq *= lacunarity;
        amp *= gain;
		erodeScale *= 0.85;
    }

    return vec3(v, g) / div;
}

vec3 fbmTest( in vec2 x, in int octaves )
{
    float f = 1.98;  // could be 2.0
    float s = 0.49;  // could be 0.5
    float a = 0.0;
    float b = 0.5;
    vec2  d = vec2(0.0,0.0);
    mat2  m = mat2(1.0,0.0,
                   0.0,1.0);
    for( int i=0; i<octaves; i++ )
    {
        vec3 n = noised(x);
        a += b*n.x;          // accumulate values
        d += b*m*n.yz;      // accumulate derivatives
        b *= s;
        x = f*m*x;
        m = f*m * i*m;
    }
    return vec3( a, d );
}

//const mat2 m = mat2(0.8,-0.6,0.6,0.8);
const mat2 m = mat2(0.825,-0.6,0.6,0.825);
const mat2 mi = mat2(0.8,0.6,-0.6,0.8);

float terrain(in vec2 p, int octaves)
{
    float a = 0.0;
    float b = 1.0;
    vec2  d = vec2(0.0,0.0);
	float total = 0.0;
    for( int i=0; i<octaves; i++ )
    {
        vec3 n=valueNoise2D_deriv_tiled(p, 10);
        d += n.yz;
        a += b*n.x/(1.0+dot(d,d));
		total += b;
        b *= 0.5;
        p = m*p*2.0;
    }
    return a / total;
}

vec3 terrainDiv( in vec2 p )
{
    float a = 0.0;
	vec2 ga = vec2(0.0);
    float b = 1.0;
    vec2  d = vec2(0.0,0.0);
	float total = 0.0;
	mat2 j = mat2(1.0);
    for( int i=0; i<15; i++ )
    {
        vec3 n=valueNoise2D_deriv_tiled(p, 1000);
        //vec3 n=noised(p);

		vec2 grad_p = j * n.yz;

        d += n.yz;

		float w = b / (1.0 + dot(d, d));

        a += w * n.x;
		ga += w * grad_p;

		total += b;
        b *= 0.5;

        p = m * p * 2.0;
		j = inverse(m) * 2.0 * j;
    }

	a /= total;
	ga /= total;
	//ga *= 4;
    return vec3(a, ga);
}

vec3 ErosionFbm(vec2 x, int octaves, float lacunarity, float gain, bool erode)
{
    float amp = 1.0;
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
		float steepness = 1.0 / (1.0 + length(nd.yz));
		if (!erode) steepness = 1.0;
		v += amp * nd.x * steepness;
        g += amp * nd.yz * freq * steepness; // chain rule
		div += amp;

        freq *= lacunarity;
        amp *= gain;
		erodeScale *= 0.85;
    }

    return vec3(v, g) / div;
}

vec3 valueNoiseFbmErosion(vec2 x, int octaves, float lacunarity, float gain, bool erode)
{
    float amp = 1.0;
    //float freq = 2.5;
    float freq = 1.0;
	float div = 0.0;
	//vec3 erosionVector = vec3(0.0);
	vec2 erosionVector = vec2(0.0);
	float erodeScale = 1.0;

    float v = 0.0;
    vec2  gr = vec2(0.0);

    for (int i = 0; i < octaves; ++i)
	{
    	// vec3 nd = valueNoise2D_deriv_tiled(x * freq, 10000);
		//vec3 nd = noised(x * freq);
		Noise2D noise = perlin2D_withHessian(x * freq);
        //vec3 nd = vec3(noise.val, noise.grad);

		float h = noise.val * 0.5 + 0.5;;
		float hx = noise.grad.x * 0.5;
		float hz = noise.grad.y * 0.5;

		float hxx = noise.hess[0][0] * 0.5;
		float hxz = noise.hess[0][1] * 0.5;
		float hzx = noise.hess[1][0] * 0.5;
		float hzz = noise.hess[1][1] * 0.5;

		erosionVector += vec2(hx, hz);
		float r = length(erosionVector);
		float steepness = 1.0 / (1.0 + r * 0.25);

		float g = steepness * h;
		float re = max(r, 1e-6);
		float inv = 1.0 / (r * (1.0 + r) * (1.0 + r));

		float gx = steepness * hx - h * (hx * hxx + hz * hxz) * inv;
		float gz = steepness * hz - h * (hx * hxz + hz * hzz) * inv;

		//erosionVector += DerivativeToNormal(nd.yz);
		//float smoothness = dot(normalize(erosionVector), normalize(vec3(0, 1, 0))) * 0.5 + 0.5;
		//smoothness = 1.0 / (1.0 + length(nd.yz));
		//v += amp * nd.x * (erode ? smoothness : 1.0);
        //g += amp * nd.yz * freq * (erode ? smoothness : 1.0); // chain rule
		v += amp * g;
        gr += amp * vec2(gx, gz) * freq; // chain rule
		div += amp;

        freq *= lacunarity;
        amp *= gain;
		//erodeScale *= 0.85;
    }

    //return vec3(v, gr);
    return vec3(v, gr) / div;
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

float HeightPower(float height, float power)
{
	//height = height * 2.0 - 1.0;
	//float heightSign = sign(height);
	//float result = clamp(pow(abs(height), power) * heightSign, 0.0, 1.0);
	//result *= heightSign;

	return (pow(height, power));
}

vec3 TerrainData(vec2 uv, int octaves, float sampleDis, bool heightOnly)
{
	//vec3 noise = valueNoiseFbm(uv + 17, 7, 4, 0.2, erode);
	octaves = clamp(octaves, 1, 20);
	float noise = terrain(uv, octaves);
	//vec3 noise = terrainDiv(uv);
	//vec3 noise = ErosionFbm(uv + 17, 7, 4, 0.2, erode);
	//vec3 noise = valueNoiseFbmErosion(uv, 4, 4, 0.2, erode);
	//vec3 noise = fbm(uv, 5, 2, 0.5);
	//noise.x += 0.5;

	const float power = 1.5;
	//const float power = 1.0;
	//float height = pow(noise.x, power);
	//float hx = power * pow(noise.x, power - 1) * noise.y;
	//float hz = power * pow(noise.x, power - 1) * noise.z;

	//float height = exp(-power * noise.x);
	//float hx = -power * exp(-power * noise.x) * noise.y;
	//float hz = -power * exp(-power * noise.x) * noise.z;

	//float height = noise.x;
	float height = HeightPower(noise, power);
	vec2 derivatives = vec2(0.0);

	if (!heightOnly)
	{
		//float dis = 0.0001;
		float dis = sampleDis;

		float noiserx;
		float noiselx;
		float noiseuy;
		float noisedy;

		//float noiserx = terrain(uv + (vec2(dis, 0)));
		//float noiselx = terrain(uv + (vec2(-dis, 0)));
		//float noiseuy = terrain(uv + (vec2(0, dis)));
		//float noisedy = terrain(uv + (vec2(0, -dis)));

		//noiserx = pow(terrain(uv + (vec2(dis, 0)), octaves), power);
		noiserx = HeightPower(terrain(uv + (vec2(dis, 0)), octaves), power);
		//if (quality) noiselx = pow(terrain(uv + (vec2(-dis, 0)), octaves), power);
		//noiseuy = pow(terrain(uv + (vec2(0, dis)), octaves), power);
		noiseuy = HeightPower(terrain(uv + (vec2(0, dis)), octaves), power);
		//if (quality) noisedy = pow(terrain(uv + (vec2(0, -dis)), octaves), power);
		//if (quality) derivatives = vec2((noiserx - noiselx) / (dis * 2), (noiseuy - noisedy) / (dis * 2));
		derivatives = vec2((noiserx - height) / (dis), (noiseuy - height) / (dis));

		//derivatives = noise.yz;
	}

	return (vec3(height, derivatives));
}

#endif