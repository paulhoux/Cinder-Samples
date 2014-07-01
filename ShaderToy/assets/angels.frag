// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

float hash( float n )
{
    return fract(sin(n)*43758.5453);
}

vec3 hash3( float n )
{
    return fract(sin(vec3(n,n+1.0,n+2.0))*vec3(43758.5453123,22578.1459123,19642.3490423));
}

// ripped from Kali's Lonely Tree shader
mat3 rotationMat(vec3 v, float angle)
{
	float c = cos(angle);
	float s = sin(angle);
	
	return mat3(c + (1.0 - c) * v.x * v.x, (1.0 - c) * v.x * v.y - s * v.z, (1.0 - c) * v.x * v.z + s * v.y,
		(1.0 - c) * v.x * v.y + s * v.z, c + (1.0 - c) * v.y * v.y, (1.0 - c) * v.y * v.z - s * v.x,
		(1.0 - c) * v.x * v.z - s * v.y, (1.0 - c) * v.y * v.z + s * v.x, c + (1.0 - c) * v.z * v.z
		);
}

vec3 axis = normalize( vec3(-0.3,-1.,-0.4) );

vec2 map( vec3 p )
{
	
    // animation	
	float atime = iGlobalTime+12.0;
	vec3 o = floor( 0.5 + p/50.0  );
	float o1 = hash( o.x*57.0 + 12.1234*o.y  + 7.1*o.z );
	float f = sin( 1.0 + (2.0*atime + 31.2*o1)/2.0 );
	p.y -= 2.0*(atime + f*f);
	p = mod( (p+25.0)/50.0, 1.0 )*50.0-25.0;
	if( abs(o.x)>0.5 )  p += (-1.0 + 2.0*o1)*10.0;
	mat3 roma = rotationMat(axis, 0.34 + 0.07*sin(31.2*o1+2.0*atime + 0.1*p.y) );

    // modeling	
	for( int i=0; i<16; i++ ) 
	{
		p = roma*abs(p);
		p.y-= 1.0;
	}
	
	float d = length(p*vec3(1.0,0.1,1.0))-0.75;
	float h = 0.5 + p.z;
		
	return vec2( d, h );
}

vec3 intersect( in vec3 ro, in vec3 rd )
{
	float maxd = 140.0;
	float precis = 0.001;
    float h=precis*2.0;
    float t = 0.0;
	float d = 0.0;
    float m = 1.0;
    for( int i=0; i<128; i++ )
    {
        if( abs(h)<precis||t>maxd ) continue;//break;
        t += h;
	    vec2 res = map( ro+rd*t );
        h = min( res.x, 5.0 );
		d = res.y;
    }

    if( t>maxd ) m=-1.0;
    return vec3( t, d, m );
}

vec3 calcNormal( in vec3 pos )
{
    vec3 eps = vec3(0.2,0.0,0.0);

	return normalize( vec3(
           map(pos+eps.xyy).x - map(pos-eps.xyy).x,
           map(pos+eps.yxy).x - map(pos-eps.yxy).x,
           map(pos+eps.yyx).x - map(pos-eps.yyx).x ) );
}

float softshadow( in vec3 ro, in vec3 rd, float mint, float k )
{
    float res = 1.0;
    float t = mint;
    for( int i=0; i<48; i++ )
    {
        float h = map(ro + rd*t).x;
		h = max( h, 0.0 );
        res = min( res, k*h/t );
        t += clamp( h, 0.01, 0.5 );
    }
    return clamp(res,0.0,1.0);
}

float calcAO( in vec3 pos, in vec3 nor )
{
	float totao = 0.0;
    for( int aoi=0; aoi<16; aoi++ )
    {
		vec3 aopos = -1.0+2.0*hash3(float(aoi)*213.47);
		aopos *= sign( dot(aopos,nor) );
		aopos = pos + aopos*0.5;
        float dd = clamp( map( aopos ).x*4.0, 0.0, 1.0 );
        totao += dd;
    }
	totao /= 16.0;
	
    return clamp( totao*totao*1.5, 0.0, 1.0 );
}

vec3 lig = normalize(vec3(-0.5,0.7,-1.0));


void main(void)
{
	vec2 q = gl_FragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0 * q;
    p.x *= iResolution.x/iResolution.y;
    vec2 m = vec2(0.5);
	if( iMouse.z>0.0 ) m = iMouse.xy/iResolution.xy;
	
    // camera
	float an = 2.5 + 0.12*iGlobalTime - 6.2*m.x;
	float cr = 0.3*cos(0.2*iGlobalTime);
    vec3 ro = vec3(15.0*sin(an),12.0-24.0*m.y,15.0*cos(an));
    vec3 ta = vec3( 0.0, 2.0, 0.0 );
    vec3 ww = normalize( ta - ro );
    vec3 uu = normalize( cross(ww,vec3(sin(cr),cos(cr),0.0) ) );
    vec3 vv = normalize( cross(uu,ww));
    float r2 = p.x*p.x*0.32 + p.y*p.y;
    p *= (7.0-sqrt(37.5-11.5*r2))/(r2+1.0);
    vec3 rd = normalize( p.x*uu + p.y*vv + 1.5*ww );

	// render
	vec3 bgc = 0.6*vec3(0.8,0.9,1.0)*(0.5 + 0.3*rd.y);
    vec3 col = bgc;

	// raymarch
    vec3 tmat = intersect(ro,rd);
    if( tmat.z>-0.5 )
    {
        // geometry
        vec3 pos = ro + tmat.x*rd;
        vec3 nor = calcNormal(pos);

        // material
		vec3 mate = 0.5 + 0.5*mix( sin( vec3(1.2,1.1,1.0)*tmat.y*3.0 ),
		                           sin( vec3(1.2,1.1,1.0)*tmat.y*6.0 ), 
								   1.0-abs(nor.y) );
		// lighting
		float occ = calcAO( pos, nor );
        float amb = 0.8 + 0.2*nor.y;
        float dif = max(dot(nor,lig),0.0);
        float bac = max(dot(nor,normalize(vec3(-lig.x,0.0,-lig.z))),0.0);
		float sha = 0.0; if( dif>0.001 ) sha=softshadow( pos, lig, 0.1, 32.0 );
        float fre = pow( clamp( 1.0 + dot(nor,rd), 0.0, 1.0 ), 2.0 );

		// lights
		vec3 brdf = vec3(0.0);
        brdf += 1.0*dif*vec3(1.00,0.90,0.65)*pow(vec3(sha),vec3(1.0,1.2,1.5));
		brdf += 1.0*amb*vec3(0.05,0.05,0.05)*occ;
		brdf += 1.0*bac*vec3(0.03,0.03,0.03)*occ;
        brdf += 1.0*fre*vec3(1.00,0.70,0.40)*occ*(0.2+0.8*sha);
        brdf += 1.0*occ*vec3(1.00,0.70,0.30)*occ*max(dot(-nor,lig),0.0)*pow(clamp(dot(rd,lig),0.0,1.0),64.0)*tmat.y*2.0;
		
        // surface-light interacion		
		col = mate * brdf;

        // fog		
		col = mix( col, bgc, clamp(1.0-1.2*exp(-0.0002*tmat.x*tmat.x ),0.0,1.0) );
    }
	else
	{
        // sun		
	    vec3 sun = vec3(1.0,0.8,0.5)*pow( clamp(dot(rd,lig),0.0,1.0), 32.0 );
	    col += sun;
	}

    // sun scatter	
	col += 0.6*vec3(0.2,0.14,0.1)*pow( clamp(dot(rd,lig),0.0,1.0), 5.0 );


	// postprocessing
	
    // gamma
	col = pow( col, vec3(0.45) );
	
	// contrast/brightness
	col = 1.3*col-0.1;
	
    // tint	
	col *= vec3( 1.0, 1.04, 1.0);
	
	// vigneting
	col *= pow( 16.0*q.x*q.y*(1.0-q.x)*(1.0-q.y), 0.1 );
	
    gl_FragColor = vec4( col, 1.0 );
}
