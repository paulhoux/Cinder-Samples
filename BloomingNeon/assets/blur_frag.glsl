#version 110

uniform sampler2D	tex0;
uniform vec2		sampleOffset;

void main()
{ 
	vec3 sum = vec3( 0.0, 0.0, 0.0 );
	sum += texture2D( tex0, gl_TexCoord[0].st + -10.0 * sampleOffset ).rgb * 0.009167927656011385;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -9.0 * sampleOffset ).rgb * 0.014053461291849008;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -8.0 * sampleOffset ).rgb * 0.020595286319257878;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -7.0 * sampleOffset ).rgb * 0.028855245532226279;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -6.0 * sampleOffset ).rgb * 0.038650411513543079;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -5.0 * sampleOffset ).rgb * 0.049494378859311142;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -4.0 * sampleOffset ).rgb * 0.060594058578763078;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -3.0 * sampleOffset ).rgb * 0.070921288047096992;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -2.0 * sampleOffset ).rgb * 0.079358891804948081;
	sum += texture2D( tex0, gl_TexCoord[0].st +  -1.0 * sampleOffset ).rgb * 0.084895951965930902;
	sum += texture2D( tex0, gl_TexCoord[0].st +   0.0 * sampleOffset ).rgb * 0.086826196862124602;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +1.0 * sampleOffset ).rgb * 0.084895951965930902;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +2.0 * sampleOffset ).rgb * 0.079358891804948081;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +3.0 * sampleOffset ).rgb * 0.070921288047096992;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +4.0 * sampleOffset ).rgb * 0.060594058578763078;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +5.0 * sampleOffset ).rgb * 0.049494378859311142;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +6.0 * sampleOffset ).rgb * 0.038650411513543079;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +7.0 * sampleOffset ).rgb * 0.028855245532226279;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +8.0 * sampleOffset ).rgb * 0.020595286319257878;
	sum += texture2D( tex0, gl_TexCoord[0].st +  +9.0 * sampleOffset ).rgb * 0.014053461291849008;
	sum += texture2D( tex0, gl_TexCoord[0].st + +10.0 * sampleOffset ).rgb * 0.009167927656011385;

	gl_FragColor.rgb = sum;
	gl_FragColor.a = 1.0;
}