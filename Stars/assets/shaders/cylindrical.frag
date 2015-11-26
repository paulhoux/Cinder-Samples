/*
 Copyright (c) 2013, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Portions of this code (C) Robert Hodgin.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and
 the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
 the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#version 150

uniform sampler2D tex;

uniform float sides = 4.0;				// = number of render views
uniform float reciprocal = 0.125;		// = 0.5 / sides
uniform float radians = 6.28318530;		// = sides * horizontal FOV per view in radians

in vec2 vTexCoord0;
out vec4 oColor;

void main()
{
	//	Official method to perform cylindrical projection
	//	based on code by Robert Hodgin.

	float s = vTexCoord0.s;
	float t = vTexCoord0.t;

	float offset = floor( s * sides ) / sides;
	float azimuth = ( s - offset ) - reciprocal;
	float tangent = tan( radians * azimuth );
	float distance = sqrt( tangent * tangent + 1.0 );

	s = reciprocal * ( tangent / tan( radians * reciprocal ) + 1.0 ) + offset;
	t = distance * ( t - 0.5 ) + 0.5;

	oColor = texture( tex, vec2( s, t ) );
}
