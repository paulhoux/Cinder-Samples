#pragma once


#include "cinder/gl/gl.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Texture.h"

#include "Shader.h"

class SMAA
{
public:
	SMAA() {}
	~SMAA() {}

	void setup();
	void apply(ci::gl::Fbo& destination, ci::gl::Fbo& source);

	ci::gl::Texture& getEdgePass();
	ci::gl::Texture& getBlendPass();
private:
	void doEdgePass(ci::gl::Fbo& source);
	void doBlendPass();
private:
	ci::gl::Fbo         mFboEdgePass;
	ci::gl::Fbo         mFboBlendPass;

	// The Shader class allows us to write and use shaders with support for #include
	ShaderRef           mSMAAFirstPass;		// edge detection
	ShaderRef           mSMAASecondPass;	// blending weight calculation
	ShaderRef           mSMAAThirdPass;		// neighborhood blending

	//
	ci::gl::TextureRef  mAreaTex;
	ci::gl::TextureRef  mSearchTex;
};