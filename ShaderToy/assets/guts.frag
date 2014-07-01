// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

//#define PROCEDURAL

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

float noise( in vec2 x )
{
    vec2 p = floor(x);
    vec2 f = fract(x);
    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0;
    return mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
               mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y);
}

const mat2 ma = mat2( 0.8, -0.6, 0.6, 0.8 );

vec2 map( vec2 p )
{
	float a  = 0.7*noise(p)*6.2831*6.0; p = ma*p*3.0;
	      a += 0.3*noise(p)*6.2831*6.0;
	
	a += 0.2*iGlobalTime;
	
	return vec2( cos(a), sin(a) );
}

vec3 texture( in vec2 p )
{
	float f = 0.0;
	
	vec2 q = p;

	p *= 32.0;
    f += 0.500*noise( p ); p = ma*p*2.02;
    f += 0.250*noise( p ); p = ma*p*2.03;
    f += 0.125*noise( p ); p = ma*p*2.01;
	f /= 0.875;
	
	vec3 col = 0.53 + 0.47*sin( f*4.5 + vec3(0.0,0.65,1.1) + 0.6 );
	
	col *= 0.7*clamp( 1.65*noise( 16.0*q.yx ), 0.0, 1.0 );
	
    return col;

}

void main( void )
{
    vec2 p = gl_FragCoord.xy / iResolution.xy;
	vec2 uv = -1.0 + 2.0*p;
	uv.x *= iResolution.x / iResolution.y;
    vec2 or = uv;
	
	float acc = 0.0;
	vec3  col = vec3(0.0);
	for( int i=0; i<64; i++ )
	{
		vec2 dir = map( uv );
		
		float h = float(i)/64.0;
		float w = 1.0-h;
		#ifdef PROCEDURAL
		vec3 ttt = w*texture(0.5*uv );
		#else
		vec3 ttt = w*texture2D( iChannel2, 0.5*uv  ).xyz;
		#endif
		ttt *= mix( 0.8*vec3(0.4,0.55,0.65), vec3(1.0,0.9,0.8), 0.5 + 0.5*dot( dir, -vec2(0.707) ) );
		
		col += w*ttt;
		acc += w;
		
		uv += 0.015*dir;
	}
	col /= acc;
    

	float ll = length(uv-or);
	vec3 nor = normalize( vec3(dFdx(ll), 4.0/iResolution.x, dFdy(ll) ) );

	float tex = texture2D(iChannel2,4.0*uv + 4.0*p).x;
	vec3 bnor = normalize( vec3(dFdx(tex), 400.0/iResolution.x, dFdy(tex)) );
	nor = normalize( nor + 0.5*normalize(bnor) );

	vec2 di = map( uv );

	col *= 0.8 + 0.2*dot( di, -vec2(0.707) );
	col *= 2.5;
	col += vec3(1.0,0.5,0.2)*0.15*dot(nor,normalize(vec3(0.8,0.2,-0.8)) );
	col += 0.12*pow(nor.y,16.0);
	col += ll*vec3(1.0,0.8,0.6)*col*0.5*(1.0-pow(nor.y,1.0));
	col *= 0.5 + ll;
	col *= 0.2 + 0.8*pow( 4.0*p.x*(1.0-p.x), 0.25 );

	gl_FragColor = vec4( col, 1.0 );
}
