// Created by inigo quilez - iq/2013
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// a perspective correct triangle rasterizer, in a shader :D

mat4 setRotation( float x, float y, float z )
{
    float a = sin(x); float b = cos(x); 
    float c = sin(y); float d = cos(y); 
    float e = sin(z); float f = cos(z); 

    float ac = a*c;
    float bc = b*c;

    return mat4( d*f,      d*e,       -c, 0.0,
                 ac*f-b*e, ac*e+b*f, a*d, 0.0,
                 bc*f+a*e, bc*e-a*f, b*d, 0.0,
                 0.0,      0.0,      0.0, 1.0 );
}

mat4 setTranslation( float x, float y, float z )
{
    return mat4( 1.0, 0.0, 0.0, 0.0,
				 0.0, 1.0, 0.0, 0.0,
				 0.0, 0.0, 1.0, 0.0,
				 x,     y,   z, 1.0 );
}

struct Vertex
{
    vec3  pos; 
	vec2  uv; 
	float occ;
};

	
struct Triangle
{
    Vertex a;
    Vertex b;
    Vertex c;
    vec3 n;
};


vec4 func( in vec2 s )
{
	float r = 1.0 + 0.4*sin(20.0*s.x + 6.28318*s.y + iGlobalTime );
	
	return vec4( r*vec3( sin(3.14159*s.x)*sin(6.28318*s.y),
			             sin(3.14159*s.x)*cos(6.28318*s.y),
			             cos(3.14159*s.x) ), pow(r*0.8,4.0) );
}


Triangle calcTriangle( float u, float v, float du, float dv, int k )
{
	vec2 aUV = vec2( u,    v    );
	vec2 bUV = vec2( u+du, v+dv );
	vec2 cUV = vec2( u+du, v    );
	vec2 dUV = vec2( u,    v+dv );
	
	if( k==1 )
	{
		cUV = bUV; 
		bUV = dUV;
	}

	vec4 a = func( aUV );
	vec4 b = func( bUV );
	vec4 c = func( cUV );
	vec3 n = normalize( -cross(c.xyz-a.xyz, b.xyz-a.xyz) );
	
	return Triangle( Vertex(a.xyz, 4.0*aUV, a.w), 
					 Vertex(b.xyz, 4.0*bUV, b.w), 
					 Vertex(c.xyz, 4.0*cUV, c.w), n );
}

vec3 lig = normalize( vec3( 0.4,0.5,0.3) );

vec3 pixelShader( in vec3 nor, in float oc, in vec2 uv, vec3 di )
{
    // perform lighting/shading
    float dif = clamp( dot( nor, lig ), 0.0, 1.0 );
	vec3 brdf = vec3(0.20,0.20,0.20)*oc + 
                vec3(0.20,0.25,0.30)*oc*(0.6+0.4*nor.y) + 
                vec3(1.00,0.90,0.80)*oc*dif;
	
    float wire = 1.0 - smoothstep( 0.0, 0.03, di.x ) *
                       smoothstep( 0.0, 0.03, di.y ) *
                       smoothstep( 0.0, 0.03, di.z );
	
    vec3 material = texture2D( iChannel3, uv ).xyz;
	
    material += wire * smoothstep(0.0,0.5,sin(3.0+iGlobalTime));
	

    return sqrt( brdf * material );
}

float cross( vec2 a, vec2 b )
{
    return a.x*b.y - a.y*b.x;
}

void main(void)
{
	vec2 mo = iMouse.xy/iResolution.xy;
	mat4 mdv = setTranslation( 0.0, 0.0, -3.0 ) * 
		       setRotation( 0.6-6.0*mo.y, 0.0,  0.6 ) * 
		       setRotation( 0.0, 20.0+0.05*iGlobalTime - 6.3*mo.x, 0.0 );
	
   
    vec2 px = -1.0 + 2.0*gl_FragCoord.xy / iResolution.xy;
    px.x *= iResolution.x/iResolution.y;


    vec3 color = vec3( 0.5 + 0.1*px.y );

	// clear zbuffer
    float mindist = -1000000.0;

	// for every triangle
	float du = 1.0/8.0;
	float dv = 1.0/8.0;
    for( int k=0; k< 2; k++ )
    for( int j=0; j<8; j++ )
    for( int i=0; i<8; i++ )
	//for( int i=0; i<100; i++ )
    {
		// get the triangle
		float pu = float(i)*du;
		float pv = float(j)*dv;
		
		//float pu = mod(float(i),10.0)*du;
		//float pv = floor(float(i)/10.0)*dv;
		
		Triangle tri = calcTriangle( pu, pv, du, dv, k );

		// transform to eye space
        vec3 ep0 = (mdv * vec4(tri.a.pos,1.0)).xyz;
        vec3 ep1 = (mdv * vec4(tri.b.pos,1.0)).xyz;
        vec3 ep2 = (mdv * vec4(tri.c.pos,1.0)).xyz;
        vec3 nor = (mdv * vec4(tri.n,0.0)).xyz;

        // transform to clip space
        float w0 = 1.0/ep0.z;
        float w1 = 1.0/ep1.z;
        float w2 = 1.0/ep2.z;

        vec2 cp0 = 2.0*ep0.xy * -w0;
        vec2 cp1 = 2.0*ep1.xy * -w1;
        vec2 cp2 = 2.0*ep2.xy * -w2;
		
		//vec2 bboxmin = min( min( cp0, cp1 ), cp2 );
		//vec2 bboxmax = max( max( cp0, cp1 ), cp2 );
        //if( px.x>bboxmin.x && px.x<bboxmax.x && px.y>bboxmin.y && px.y<bboxmax.y )
		{
		
        // fetch vertex attributes, and divide by z
        vec2  u0 = tri.a.uv  * w0;
        vec2  u1 = tri.b.uv  * w1;
        vec2  u2 = tri.c.uv  * w2;
        float a0 = tri.a.occ * w0;
        float a1 = tri.b.occ * w1;
        float a2 = tri.c.occ * w2;

        //-----------------------------------
        // rasterize
        //-----------------------------------

        // calculate areas for subtriangles
        vec3 di = vec3( cross( cp1 - cp0, px - cp0 ), 
					    cross( cp2 - cp1, px - cp1 ), 
					    cross( cp0 - cp2, px - cp2 ) );
			
        // if all positive, point is inside triangle
        if( all(greaterThan(di,vec3(0.0))) )
        {
            // calc barycentric coordinates
            vec3 ba = di.yzx / (di.x+di.y+di.z);

            // barycentric interpolation of attributes and 1/z
            float iz = ba.x*w0 + ba.y*w1 + ba.z*w2;
            vec2  uv = ba.x*u0 + ba.y*u1 + ba.z*u2;
            float oc = ba.x*a0 + ba.y*a1 + ba.z*a2;

            // recover interpolated attributes (this could be done after 1/depth test)
			float z = 1.0/iz;
            uv *= z;
			oc *= z;

			// depth buffer test
			if( z>mindist )
			{
				mindist = z;

                // run pixel shader
                color = pixelShader( nor, oc, uv, ba );
			}
        }
		}
    }

	color *= 1.0 - 0.1*dot(px,px);
    color.y *= 1.03;
	
    gl_FragColor = vec4(color,1.0);
}