/*
 Copyright (c) 2014, Paul Houx - All rights reserved.
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

#include "Pistons.h"

#include "cinder/Camera.h"
#include "cinder/Rand.h"
#include "cinder/app/App.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;

Piston::Piston()
    : mOffset( 0.0f )
    , mColor( 1.0f, 1.0f, 1.0f )
    , mPosition( 0.0f, 0.0f, 0.0f )
{
}

Piston::Piston( float x, float z )
    : mOffset( ci::Rand::randFloat( 0.0f, 10.0f ) )
    , mColor( ci::Color( ci::CM_HSV, ci::Rand::randFloat( 0.0f, 0.1f ), ci::Rand::randFloat( 0.0f, 1.0f ), ci::Rand::randFloat( 0.25f, 1.0f ) ) )
    , mPosition( ci::vec3( x, 0.0f, z ) )
{
}

void Piston::update( const ci::Camera &camera, float time )
{
	float t = mOffset + time;
	float height = 55.0f + 45.0f * ci::math<float>::sin( t );
	mPosition.y = 0.5f * height;

	mDistance = glm::distance2( mPosition, camera.getEyePoint() );
}

/////////////////////////////////////

const char *Pistons::vs
    = "#version 150\n"
      ""
      "uniform mat4 ciModelView;\n"
      "uniform mat4 ciModelViewProjection;\n"
      "uniform mat3 ciNormalMatrix;\n"
      ""
      "in vec4 ciPosition;\n"
      "in vec3 ciNormal;\n"
      "in vec2 ciTexCoord0;\n"
      "in vec4 ciColor;\n"
      ""
      "in vec4 iPosition;\n" // xyz = position, w = distance
      "in vec4 iColor;\n"    // xyz = color, w = offset
      ""
      "out vec4 vertPosition;\n"
      "out vec3 vertNormal;\n"
      "out vec2 vertTexCoord0;\n"
      "out vec3 vertColor;\n"
      ""
      "void main()\n"
      "{\n"
      "	vec4 scale = vec4( 10, 2.0 * iPosition.y, 10, 1 );\n"
      "	vec4 position = ciPosition * scale + vec4( iPosition.xyz, 0.0 );\n"
      ""
      "	vertPosition = ciModelView * position;\n"
      "	vertNormal = ciNormalMatrix * ciNormal;\n"
      "	vertTexCoord0 = ciTexCoord0;\n"
      "	vertColor = iColor.rgb;\n"
      ""
      "	gl_Position = ciModelViewProjection * position;\n"
      "}";

const char *Pistons::fs
    = "#version 150\n"
      ""
      "in vec4 vertPosition;\n"
      "in vec3 vertNormal;\n"
      "in vec2 vertTexCoord0;\n"
      "in vec3 vertColor;\n"
      ""
      "out vec4 fragColor;\n"
      ""
      "void main()\n"
      "{\n"
      "	vec2 uv = vertTexCoord0;\n"
      ""
      "	vec3 N = normalize(vertNormal);\n"
      "	vec3 L = normalize(-vertPosition.xyz);\n"
      "	vec3 E = normalize(-vertPosition.xyz);\n"
      "	vec3 R = normalize(-reflect(L,N));\n"

      // diffuse term with fake ambient occlusion
      "	float occlusion = 0.5 + 0.5*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y);\n"
      "	vec3 diffuse = vertColor * occlusion;\n"
      "	diffuse *= max(dot(N,L), 0.0);\n"

      // specular term
      "	vec3 specular = vertColor;\n"
      "	specular *= pow(max(dot(R,E),0.0), 50.0);\n"

      // write gamma corrected final color
      "	vec3 final = sqrt(diffuse + specular);\n"
      "	fragColor.rgb = final;\n"

      // write luminance in alpha channel (required for FXAA)
      "	const vec3 luminance = vec3(0.299, 0.587, 0.114);\n"
      "	fragColor.a = dot( final, luminance );\n"
      "}";

void Pistons::setup()
{
	mInstances.clear();

	Rand::randSeed( 2015 );
	for( int x = -50; x <= 50; x += 10 )
		for( int z = -50; z <= 50; z += 10 )
			mInstances.emplace_back( Piston( float( x ), float( z ) ) );

	// Load and compile our shaders and textures
	try {
		geom::BufferLayout instanceDataLayout;
		instanceDataLayout.append( geom::Attrib::CUSTOM_0, 4, sizeof( Piston ), offsetof( Piston, mPosition ), 1 /* per instance */ );
		instanceDataLayout.append( geom::Attrib::CUSTOM_1, 4, sizeof( Piston ), offsetof( Piston, mColor ), 1 /* per instance */ );

		mInstanceVbo = gl::Vbo::create( GL_ARRAY_BUFFER, mInstances.size() * sizeof( Piston ), mInstances.data(), GL_DYNAMIC_DRAW );
		auto glsl = gl::GlslProg::create( vs, fs );
		auto mesh = gl::VboMesh::create( geom::Cube() );
		mesh->appendVbo( instanceDataLayout, mInstanceVbo );

		mBatch = gl::Batch::create( mesh, glsl, { { geom::Attrib::CUSTOM_0, "iPosition" }, { geom::Attrib::CUSTOM_1, "iColor" } } );
	}
	catch( const std::exception &e ) {
		console() << e.what() << std::endl;
	}
}

void Pistons::update( const ci::Camera &camera, float time )
{
	for( auto &instance : mInstances )
		instance.update( camera, time );

	std::qsort( &mInstances.front(), mInstances.size(), sizeof( Piston ), &Piston::CompareByDistanceToCamera );

	Piston *ptr = static_cast<Piston *>( mInstanceVbo->mapReplace() );
	for( size_t i = 0; i < mInstances.size(); ++i ) {
		*ptr++ = mInstances[i];
	}
	mInstanceVbo->unmap();
}

void Pistons::draw( const ci::Camera &camera )
{
	gl::ScopedDepth       scpDepth( true );
	gl::ScopedFaceCulling scpCull( true, GL_BACK );
	gl::ScopedColor       scpColor( Color::white() );
	gl::ScopedBlend       scpBlend( false );

	gl::pushMatrices();
	gl::setMatrices( camera );

	mBatch->drawInstanced( mInstances.size() );

	gl::popMatrices();
}