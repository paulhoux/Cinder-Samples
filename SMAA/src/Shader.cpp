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

#include "Shader.h"

#include "cinder/Utilities.h"
#include "cinder/app/AppBasic.h"

#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/tokenizer.hpp>

using namespace ci;

Shader::Shader(void) :
	bHasGeometryShader(false),
	mGlslVersion(0)
{
}

Shader::Shader(const std::string& name) :
	mName(name),
	mVertexFile(name + "_vert.glsl"),
	mFragmentFile(name + "_frag.glsl"),
	mGeometryFile(name + "_geom.glsl"),
	bHasGeometryShader(false),
	mGlslVersion(0)
{
	load();
}

Shader::~Shader(void)
{
}

ShaderRef Shader::create()
{
	return std::make_shared<Shader>();
}

ShaderRef Shader::create(const std::string& name)
{
	return std::make_shared<Shader>(name);
}

void Shader::load()
{
	// get a reference to our path (and create it in the process)
	const fs::path& path = getPath();
	if(path.empty()) throw ShaderNotFoundException(mName);

	// check if all files are present
	if(!fs::exists(path / mFragmentFile)) throw ShaderIncompleteException(mName);
	
	bHasGeometryShader = fs::exists(path / mGeometryFile);
	//if(bHasGeometryShader) TODO: check if geometry settings are defined

	// parse source
	std::string vertexSource = parseShader( path / mVertexFile );
	std::string fragmentSource = parseShader( path / mFragmentFile );

	try {
		if(bHasGeometryShader) {
			std::string geometrySource = parseShader( path / mGeometryFile );

			mGlslProg = gl::GlslProg( vertexSource.c_str(), fragmentSource.c_str(), geometrySource.c_str(),
										GL_TRIANGLES, GL_TRIANGLE_STRIP, 3); // TODO: support other geometry types
		}
		else {
			mGlslProg = gl::GlslProg( vertexSource.c_str(), fragmentSource.c_str() );
		}
	}
	catch( const std::exception& e ) {
		throw ShaderCompileException(mName, std::string(e.what()));
	}
}

const fs::path& Shader::getPath() const
{
	// return path if already found
	if( !mPath.empty() ) return mPath;

	// find path:
	//  1. in assets folder
	//  2. in assets/shaders folder
	//  3. next to executable
	mPath = app::getAssetPath("");
	if( fs::exists(mPath / mVertexFile) ) return mPath;

	mPath = app::getAssetPath("") / "shaders";
	if( fs::exists(mPath / mVertexFile) ) return mPath;

	mPath = app::getAppPath();
	if( fs::exists(mPath / mVertexFile) ) return mPath;

	// not found
	mPath.clear();
	return mPath;
}

std::string Shader::parseShader( const fs::path& path, bool optional, int level )
{
	std::stringstream output;

	if( level > 32 )
	{
		throw std::exception("Reached the maximum inclusion depth.");
		return std::string();
	}

	static const boost::regex includeRegexp( "^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*" );

	std::ifstream input( path.c_str() );
	if( !input.is_open() )
	{
		if( optional )
			return std::string();

		if( level == 0 )
			throw std::exception("Failed to open shader file.");
		else
			throw std::exception("Failed to open shader include file.");

		return std::string();
	}

	// go through each line and process includes
	std::string		line;
	boost::smatch	matches;

	while( std::getline( input, line ) )
	{
		if( boost::regex_search( line, matches, includeRegexp ) )
			output << parseShader( path.parent_path() / matches[1].str(), false, level + 1 );
		else
			output << line;

		output << std::endl;
	}

	input.close();

		// make sure #version is the first line of the shader
	if( level == 0)
		return parseVersion( output.str() );
	else
		return output.str();
}

std::string Shader::parseVersion( const std::string& code )
{
    static const boost::regex versionRegexp( "^[ ]*#[ ]*version[ ]+([123456789][0123456789][0123456789]).*$" );

	if(code.empty())
		return std::string();
       
    mGlslVersion = 120;  

    std::stringstream completeCode( code );
    std::ostringstream cleanedCode;

    std::string line;
    boost::smatch matches; 
    while( std::getline( completeCode, line ) )
    {
        if( boost::regex_match( line, matches, versionRegexp ) ) 
        {
			unsigned int versionNum = ci::fromString< unsigned int >( matches[1] );	
            mGlslVersion = std::max( versionNum, mGlslVersion );

            continue;
        }

        cleanedCode << line << std::endl;
    }
	
    std::stringstream vs;
    vs << "#version " << mGlslVersion << std::endl << cleanedCode.str();

    return vs.str();
}