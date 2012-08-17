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

#include "cinder/Rand.h"
#include "text/Text.h"

#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

namespace ph { namespace text {

using namespace ci;

void Text::draw()
{
	render();

	if(mVboMesh && mFont) {
		mFont->enableAndBind();
		gl::draw(mVboMesh);
		mFont->unbind();
	}
}

void Text::drawWireframe()
{
	render();
 
	if(!mVboMesh) return;

	glPushAttrib( GL_POLYGON_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT );

	gl::enableWireframe();
	gl::disable( GL_TEXTURE_2D );

	gl::draw(mVboMesh);

	glPopAttrib();
}

// This is not a pretty piece of code, nor is it blazingly fast. But it works for now.
void Text::render()
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
	//std::vector<ColorA>		colors;

	std::vector<Vec2f>		vecLine, vecWord, texLine, texWord;
	std::vector<Vec2f>::const_iterator	iter;

	Vec2f					cword;
	
	// initialize cursor position
	Vec2f cursor(0.0f, std::floorf(mFont->getAscent(mFontSize) + 0.5f));

	// prevent errors
	if( mText.empty() ) {
		mVboMesh.reset();
		mInvalid = true;

		return;
	}

	// use the boost tokenizer to split the text into lines without copying all of it
	std::wstring::const_iterator itr;
	boost::split_iterator<std::wstring::iterator> lineItr, wordItr, endItr;
	for (lineItr=boost::make_split_iterator(mText, boost::token_finder(boost::is_any_of(L"\n")));lineItr!=endItr;++lineItr) {
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
		}

		// next line
		if(!newLine(&cursor)) break;
	}

	//
	gl::VboMesh::Layout layout;
	layout.setStaticPositions();
	layout.setStaticIndices();
	layout.setStaticTexCoords2d();
	//layout.setStaticColorsRGBA();

	mVboMesh = gl::VboMesh( vertices.size(), indices.size(), layout, GL_TRIANGLES );
	mVboMesh.bufferPositions( &vertices.front(), vertices.size() );
	mVboMesh.bufferIndices( indices );
	mVboMesh.bufferTexCoords2d( 0, texcoords );
	//mVboMesh.bufferColorsRGBA( colors );

	mInvalid = false;
}

Rectf Text::getBounds()
{
	if(mBoundsInvalid) {
		//mBounds = mMesh.calcBoundingBox();
		mBoundsInvalid = false;
	}

	return mBounds;
}

} } // namespace ph::text