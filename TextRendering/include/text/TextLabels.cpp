/*
Copyright (C) 2011-2012 Paul Houx

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "text/TextLabels.h"

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

namespace ph { namespace text {

using namespace ci;
using namespace std;

void TextLabels::clear()
{
	mLabels.clear();
	mInvalid = true;
}

void TextLabels::addLabel( const Vec3f &position, const std::wstring &text )
{
	mLabels.insert( pair<Vec3f, std::wstring>( position, text ) );
	mInvalid = true;
}

void TextLabels::render()
{
	if(!mInvalid) return;
	if(!mFont) return;

	//
	float					space = mFont->getAdvance(32, mFontSize);
	float					width;
	float					height = getHeight();
	Rectf					bounds;

	uint32_t				index = 0;
	uint16_t				id = 0;

	std::vector<Vec3f>		vertices;
	std::vector<uint32_t>	indices;
	std::vector<Vec2f>		texcoords;
	std::vector<Vec3f>		offsets;

	std::vector<Vec2f>		vecLine, vecWord, texLine, texWord;
	std::vector<Vec2f>::const_iterator	iter;

	// prevent errors
	if( mLabels.empty() ) {
		mVboMesh.reset();
		mInvalid = true;

		return;
	}

	// parse all labels
	TextLabelListIter labelItr;
	for(labelItr=mLabels.begin();labelItr!=mLabels.end();++labelItr) {
		Vec3f			offset = labelItr->first;
		std::wstring	text = labelItr->second;

		Vec2f					cword;
	
		// initialize cursor position
		Vec2f cursor( mOffset.x, mOffset.y + std::floorf(mFont->getAscent(mFontSize) + 0.5f));

		// use the boost tokenizer to split the text into lines without copying all of it
		std::wstring::const_iterator itr;
		boost::split_iterator<std::wstring::iterator> lineItr, wordItr, endItr;
		for (lineItr=boost::make_split_iterator(text, boost::token_finder(boost::is_any_of(L"\n")));lineItr!=endItr;++lineItr) {
			//
			if(lineItr->empty()) {
				newLine(&cursor);
				continue;
			}

			// clear the line vertices
			vecLine.clear();
			texLine.clear();

			width = getWidthAt( cursor.y );

			// use the boost tokenizer to split the line into words without copying all of it
			for (wordItr=boost::make_split_iterator(*lineItr, boost::token_finder(boost::is_any_of(L" ")));wordItr!=endItr;++wordItr) {
				//std::wstring word = boost::copy_range<std::wstring>(*wordItr);
				//if(word.empty()) continue;
				if(wordItr->empty()) continue;

				// clear the word vertices
				vecWord.clear();
				texWord.clear();

				if(cursor.x > 0.0f) cursor.x += space;
				cword = Vec2f::zero();

				// parse the word
				for(itr=wordItr->begin();itr!=wordItr->end();++itr) {
					// retrieve character code
					id = (uint16_t) *itr;

					if( mFont->contains(id) ) {
						bounds = mFont->getBounds(id, mFontSize);
						vecWord.push_back( cword + bounds.getUpperLeft() );
						vecWord.push_back( cword + bounds.getUpperRight() );
						vecWord.push_back( cword + bounds.getLowerRight() );
						vecWord.push_back( cword + bounds.getLowerLeft() );
			
						bounds = mFont->getTexCoords(id);
						texWord.push_back( bounds.getUpperLeft() );
						texWord.push_back( bounds.getUpperRight() );
						texWord.push_back( bounds.getLowerRight() );
						texWord.push_back( bounds.getLowerLeft() );

						indices.push_back(index+0); indices.push_back(index+3); indices.push_back(index+1);
						indices.push_back(index+1); indices.push_back(index+3); indices.push_back(index+2);
						index += 4;

						cword.x += mFont->getAdvance(id, mFontSize);
					}
				}

				// check if the word fits on this line
				if((cursor.x + cword.x) > width) {
					// it doesn't, finish this line and start a new one,
					// but first move it to the right for proper alignment
					if( mAlignment == CENTER ) {
						float dx = 0.5f * (width - cursor.x);
						std::vector<Vec2f>::iterator i;
						for(i=vecLine.begin();i!=vecLine.end();++i)
							i->x += dx;
					}
					else if( mAlignment == RIGHT ) {
						float dx = (width - cursor.x);
						std::vector<Vec2f>::iterator i;
						for(i=vecLine.begin();i!=vecLine.end();++i)
							i->x += dx;
					}

					vertices.insert(vertices.end(), vecLine.begin(), vecLine.end());
					texcoords.insert(texcoords.end(), texLine.begin(), texLine.end());
					//colors.insert(colors.end(), vecLine.size(), Color::white());
					offsets.insert(offsets.end(), vecLine.size(), offset);

					//
					vecLine.clear();
					texLine.clear();				

					// next line
					if(!newLine(&cursor)) break;

					width = getWidthAt( cursor.y );
				}

				// add the word to the current line
				for(iter=vecWord.begin();iter!=vecWord.end();++iter)
					vecLine.push_back(cursor + *iter);
				for(iter=texWord.begin();iter!=texWord.end();++iter)
					texLine.push_back(*iter);

				//
				vecWord.clear();
				texWord.clear();

				// 
				cursor.x += cword.x;
			}

			// stop if no more room
			if( height > 0.0f && cursor.y >= height )
				break;

			// finish this line and start a new one
			if(!vecLine.empty()) {
				// but first move it to the right for proper alignment
				if( mAlignment == CENTER ) {
					float dx = 0.5f * (width - cursor.x);
					std::vector<Vec2f>::iterator i;
					for(i=vecLine.begin();i!=vecLine.end();++i)
						i->x += dx;
				}
				else if( mAlignment == RIGHT ) {
					float dx = (width - cursor.x);
					std::vector<Vec2f>::iterator i;
					for(i=vecLine.begin();i!=vecLine.end();++i)
						i->x += dx;
				}

				vertices.insert(vertices.end(), vecLine.begin(), vecLine.end());
				texcoords.insert(texcoords.end(), texLine.begin(), texLine.end());
				//colors.insert(colors.end(), vecLine.size(), Color::white());
				offsets.insert(offsets.end(), vecLine.size(), offset);
			}

			// next line
			if(!newLine(&cursor)) break;
		}


	}

	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d(0);
	//layout.setStaticColorsRGBA();
	layout.setStaticTexCoords3d(1);

	mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_TRIANGLES );
	mVboMesh.bufferPositions( &vertices.front(), vertices.size() );
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texcoords );
	//mVboMesh.bufferColorsRGBA( colors );
	mVboMesh.bufferTexCoords3d( 1, offsets );

	mInvalid = false;
}

std::string TextLabels::getVertexShader() const
{
	// fragment shader
	const char *vs = 
		"#version 120\n"
		"\n"
		"uniform vec2 viewportSize;\n"
		"\n"
		"vec3 toClipSpace(vec4 vertex)\n"
		"{\n"
		"	return vec3( vertex.xyz / vertex.w );\n"
		"}\n"
		"\n"
		"vec2 toScreenSpace(vec4 vertex)\n"
		"{\n"
		"	return vec2( vertex.xy / vertex.w ) * viewportSize;\n"
		"}\n"
		"\n"
		"void main()\n"
		"{\n"
		"	// pass font texture coordinate to fragment shader\n"
		"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
		"\n"
		"	// retrieve label position\n"
		"	vec4 position = ( gl_ModelViewProjectionMatrix * gl_MultiTexCoord1 );\n"
		"\n"
		"	// convert position to clip space to find the 2D offset\n"
		"	vec3 offset = toClipSpace( position );\n"
		"\n"
		"	// if the label is behind us, then don't draw the label (set alpha to zero)\n"
		"	float a = (position.w < 0) ? 0.0 : gl_Color.a;\n"
		"	gl_FrontColor = vec4( gl_Color.rgb, a );\n"
		"\n"
		"	// calculate vertex position by offsetting it\n"
		"	gl_Position = vec4( (gl_Vertex.xy * vec2(1.0, -1.0)) / viewportSize * 2.0 + offset.xy, 0.0, 1.0 );\n"
		"}";

	return std::string(vs);
}

bool TextLabels::bindShader()
{
	if( Text::bindShader() )
	{
		mShader.uniform( "viewportSize", Vec2f( app::getWindowSize() ) );
		return true;
	}

	return false;
}

} } // namespace ph::text