/*
 Copyright (c) 2010-2012, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

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

#include "ConstellationArt.h"
#include "Conversions.h"

#include "cinder/ImageIo.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;
using namespace std;

ConstellationArt::ConstellationArt(void)
	: mAttenuation(1.0f)
{
}

ConstellationArt::~ConstellationArt(void)
{
}

void ConstellationArt::setup()
{
	try { 
		gl::Texture::Format fmt;
		fmt.enableMipmapping(true);
		fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );

		mTexture = gl::Texture( loadImage( loadAsset("textures/constellations.jpg") ), fmt ); 
	}
	catch( const std::exception &e ) { console() << "Could not load texture: " << e.what() << std::endl; }

	try { mShader = gl::GlslProg( getVertexShader().c_str(), getFragmentShader().c_str() ); }
	catch( const std::exception &e ) { console() << "Could not load&compile shader: " << e.what() << std::endl; }

	create();
}

void ConstellationArt::draw()
{
	if(!( mTexture && mShader && mVboMesh )) return;

	glPushAttrib( GL_TEXTURE_BIT | GL_ENABLE_BIT | GL_CURRENT_BIT );

	mTexture.enableAndBind();

	gl::pushModelView();
	{
		gl::enableAdditiveBlending();
		gl::color( mAttenuation * Color(0.4f, 0.6f, 0.8f) );

		//mShader.bind();
		//mShader.uniform("map", 0);
		//mShader.uniform("smoothness", 1.0f);

		gl::draw( mVboMesh );

		//mShader.unbind();
	}
	gl::popModelView();

	glPopAttrib();
}


void ConstellationArt::create()
{
	const double	TWO_PI = 2.0 * M_PI;
	const double	HALF_PI = 0.5 * M_PI;

	const int		SLICES = 30;
	const int		SEGMENTS = 60;
	const int		RADIUS = 25;

	// create data buffers
	vector<Vec3f>		normals;
	vector<Vec3f>		positions;
	vector<Vec2f>		texCoords;
	vector<uint32_t>	indices;

	//	
	int x, y;
	for(x=0;x<=SEGMENTS;++x) {
		double theta = static_cast<double>(x) / SEGMENTS * TWO_PI;

		for(y=0;y<=SLICES;++y) {
			double phi = (0.5 - static_cast<double>(y) / SLICES) * M_PI;

			normals.push_back( Vec3f(
				static_cast<float>( cos(phi) * sin(theta) ),
				static_cast<float>( sin(phi) ),
				static_cast<float>( cos(phi) * cos(theta) ) ) );

			positions.push_back( normals.back() * RADIUS );	

			float tx = 1.0f - static_cast<float>(x) / SEGMENTS;
			float ty = static_cast<float>(y) / SLICES;

			texCoords.push_back( Vec2f(tx, ty) );
		}
	}

	//
	int rings = SLICES+1;
	bool forward = false;
	for(x=0;x<SEGMENTS;++x) {
        if(forward) {
			// create jumps in the triangle strip by introducing degenerate polygons
			indices.push_back(  x      * rings + 0 );
			indices.push_back(  x      * rings + 0 );
			// 
            for(y=0;y<rings;++y) {
                indices.push_back(  x      * rings + y );
                indices.push_back( (x + 1) * rings + y );
            }
        }
        else {
			// create jumps in the triangle strip by introducing degenerate polygons
			indices.push_back( (x + 1) * rings + SLICES );
			indices.push_back( (x + 1) * rings + SLICES );
			// 
            for(y=SLICES;y>=0;--y) {
                indices.push_back( (x + 1) * rings + y );
                indices.push_back(  x      * rings + y );
            }
        }

        forward = !forward;
    }

	// create the mesh
	gl::VboMesh::Layout layout;	
	layout.setStaticIndices();
	layout.setStaticPositions();
	layout.setStaticTexCoords2d();
	layout.setStaticNormals();

	mVboMesh = gl::VboMesh(positions.size(), indices.size(), layout, GL_TRIANGLE_STRIP);

	mVboMesh.bufferNormals( normals );
	mVboMesh.bufferTexCoords2d( 0, texCoords );
	mVboMesh.bufferPositions( positions );
	mVboMesh.bufferIndices( indices );
}

void ConstellationArt::setCameraDistance( float distance )
{
	static const float minimum = 0.01f;
	static const float maximum = 0.7f;

	mAttenuation = math<float>::clamp( 1.0f - (distance / 12.0f), minimum, maximum );
}

std::string ConstellationArt::getVertexShader() const
{
	// vertex shader
	const char *vs = 
		"#version 110\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_FrontColor = gl_Color;\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"\n"
		"	gl_Position = ftransform();\n"
		"}\n";

	return std::string(vs);
}

std::string ConstellationArt::getFragmentShader() const
{
	// fragment shader
	const char *fs = 
		"#version 110\n"
		"\n"
		"uniform sampler2D	map;\n"
		"uniform float      smoothness;\n"
		"\n"
		"const float gamma = 2.2;\n"
		"\n"
		"void main()\n"
		"{\n"
		"	// retrieve signed distance\n"
		"	float sdf = texture2D( map, gl_TexCoord[0].xy ).r;\n"
		"\n"
		"	// perform adaptive anti-aliasing of the edges\n"
		"	float w = clamp( smoothness * (abs(dFdx(gl_TexCoord[0].x)) + abs(dFdy(gl_TexCoord[0].y))), 0.0, 0.5);\n"
		"	float a = smoothstep(0.5-w, 0.5+w, sdf);\n"
		"\n"
		"	// gamma correction for linear attenuation\n"
		"	a = pow(a, 1.0/gamma);\n"
		"\n"
		"	// final color\n"
		"	gl_FragColor.rgb = gl_Color.rgb;\n"
		"	gl_FragColor.a = gl_Color.a * a;\n"
		"}\n";

	return std::string(fs);
}