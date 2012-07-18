// Post-processing shader adapted from code by Iñigo Quilez
// http://www.iquilezles.org/
// (C)2010 Iñigo Quilez 
// This implementation (C) 2011 Paul Houx

#version 110

uniform float time;
uniform sampler2D tex0;

void main(void)
{
	// initialize coordinates and colors
    vec2 q = gl_TexCoord[0].xy;
    vec2 uv = q;
    vec3 oricol = texture2D(tex0,vec2(q.x,q.y)).xyz;
    vec3 col;
	
	// zooming (disabled)
    //uv = 0.5 + (q-0.5)*(0.9 + 0.1*sin(0.2*time));

	// color separation
    col.r = texture2D(tex0,vec2(uv.x+0.003,uv.y)).x;
    col.g = texture2D(tex0,vec2(uv.x+0.000,uv.y)).y;
    col.b = texture2D(tex0,vec2(uv.x-0.003,uv.y)).z;

	// increase contrast
    col = clamp(col*0.5+0.5*col*col*1.2,0.0,1.0);

	// vignetting
    col *= 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);

	// tinting
    col *= vec3(0.8,1.0,0.7);

	// tv lines
    col *= 0.9+0.1*sin(10.0*time+uv.y*1800.0);

	// flickering
    col *= 0.97+0.03*sin(110.0*time);

	// show/hide original (disabled)
    //float comp = smoothstep( 0.2, 0.7, sin(time) );
    //col = mix( col, oricol, clamp(-2.0+2.0*q.x+3.0*comp,0.0,1.0) );

	// set final color
    gl_FragColor = vec4(col,1.0);
}