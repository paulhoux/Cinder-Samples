// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

//#define FULL_PROCEDURAL

#ifdef FULL_PROCEDURAL

// hash based 3d value noise
float hash( float n )
{
    return fract(sin(n)*43758.5453);
}
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);

    f = f*f*(3.0-2.0*f);
    float n = p.x + p.y*57.0 + 113.0*p.z;
    return mix(mix(mix( hash(n+  0.0), hash(n+  1.0),f.x),
                   mix( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
               mix(mix( hash(n+113.0), hash(n+114.0),f.x),
                   mix( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
}
#else
float noise( in vec3 x )
{
    vec3 p = floor(x);
    vec3 f = fract(x);
	f = f*f*(3.0-2.0*f);
	
	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;
	vec2 rg = texture2D( iChannel0, (uv+ 0.5)/256.0, -100.0 ).yx;
	return mix( rg.x, rg.y, f.z );
}
#endif

vec4 map( vec3 p )
{
	vec3 r = p;
	
    p.y += 0.6;	
	
    // invert space	
	p = -4.0*p/dot(p,p);

    // twist space	
	float an = -1.0*sin(0.1*iGlobalTime + length(p.xz) + p.y);
	float co = cos(an);
	float si = sin(an);
	p.xz = mat2(co,-si,si,co)*p.xz;

    // distort	
	p.xz += -1.0 + 2.0*noise( p*1.1 );

    // pattern
	float f;
	vec3 q = p*0.85                     - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;
    f  = 0.50000*noise( q ); q = q*2.02 - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;
    f += 0.25000*noise( q ); q = q*2.03 - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;
    f += 0.12500*noise( q ); q = q*2.01 - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;
    f += 0.06250*noise( q ); q = q*2.02 - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;
    f += 0.04000*noise( q ); q = q*2.00 - vec3(0.0,1.0,0.0)*iGlobalTime*0.12;

	float den = clamp( (-r.y-0.6 + 4.0*f)*1.2, 0.0, 1.0 );
	
	vec3 col = 1.2*mix( vec3(1.0,0.8,0.6), 0.9*vec3(0.3,0.2,0.35), den ) ;
	col += 0.05*sin(0.05*q);
	col *= 1.0 - 0.8*smoothstep(0.6,1.0,sin(0.7*q.x)*sin(0.7*q.y)*sin(0.7*q.z))*vec3(0.6,1.0,0.8);
	col *= 1.0 + 1.0*smoothstep(0.5,1.0,1.0-length( (fract(q.xz*0.12)-0.5)/0.5 ))*vec3(1.0,0.9,0.8) ;
	col = mix( vec3(0.8,0.32,0.2), col, clamp( (r.y+0.1)/1.5, 0.0, 1.0 ) );

	return vec4( col, den );
}


vec3 raymarch( in vec3 ro, in vec3 rd )
{
	vec4 sum = vec4( 0.0 );
	vec3 bg = vec3(0.4,0.5,0.5)*1.3;

	// dithering
	float t = 0.05*texture2D( iChannel0, gl_FragCoord.xy/iChannelResolution[0].x ).x;
	
	for( int i=0; i<128; i++ )
	{
		if( sum.a > 0.99 ) continue;
		vec3 pos = ro + t*rd;
		vec4 col = map( pos );
		
		col.rgb = mix( bg, col.rgb, exp(-0.002*t*t*t) );
		col.a *= 0.5;
		col.rgb *= col.a;

		sum = sum + col*(1.0 - sum.a);	
		
		t += 0.05;
	}

	return clamp( mix( bg, sum.xyz/(0.001+sum.w), sum.w ), 0.0, 1.0 );
}

void main(void)
{
    // inputs	
	vec2 q = gl_FragCoord.xy / iResolution.xy;
    vec2 p = (-1.0 + 2.0*q) * vec2( iResolution.x/ iResolution.y, 1.0 );
	
    vec2 mo = iMouse.xy / iResolution.xy;
    if( iMouse.w<=0.00001 ) mo=vec2(0.0);
	
    // camera
	float an = -0.07*iGlobalTime + 3.0*mo.x;
    vec3 ro = 4.5*normalize(vec3(cos(an), 0.5, sin(an)));
	ro.y += 1.0;
	vec3 ta = vec3(0.0, 0.5, 0.0);
	float cr = -0.4*cos(0.02*iGlobalTime);
	
	// build ray
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross( vec3(sin(cr),cos(cr),0.0), ww ) );
    vec3 vv = normalize( cross(ww,uu) );
    vec3 rd = normalize( p.x*uu + p.y*vv + 2.5*ww );
		
    // raymarch	
	vec3 col = raymarch( ro, rd );
	
	// contrast
	col = col*col*(3.0-2.0*col)*1.4 - 0.4;
	
    // vignetting		
	col *= 0.25 + 0.75*pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 );
	
    gl_FragColor = vec4( col, 1.0 );
}
