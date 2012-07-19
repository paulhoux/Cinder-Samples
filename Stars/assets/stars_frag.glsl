uniform sampler2D tex0;

void main() 
{
	gl_FragColor = texture2D(tex0, gl_PointCoord) * gl_Color;
}