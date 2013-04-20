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

#pragma once

// forward declarations
namespace cinder {
	class AxisAlignedBox; 
	class Camera; class CameraPersp; class CameraStereo; class CameraOrtho;
	namespace app {
		class MouseEvent; class KeyEvent; class ResizeEvent; class FileDropEvent;
	}
}

#include "cinder/Color.h"
#include "cinder/Matrix.h"
#include "cinder/Quaternion.h"
#include "cinder/Vector.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Material.h"

#include <boost/enable_shared_from_this.hpp>
#include <iostream>
#include <deque>
#include <map>

namespace ph { namespace nodes {

typedef boost::shared_ptr<class Node>		NodeRef;
typedef boost::shared_ptr<const class Node>	NodeConstRef;
typedef boost::weak_ptr<class Node>			NodeWeakRef;
typedef std::deque<NodeRef>					NodeList;
typedef std::map<unsigned int, NodeWeakRef>	NodeMap;

class Node
	: public boost::enable_shared_from_this<Node>
{
public:
	Node(void);
	virtual ~Node(void);
	
	//! sets the node's parent node (using weak reference to avoid objects not getting destroyed)
	void setParent(NodeRef node){ mParent = NodeWeakRef(node); }
	//! returns the node's parent node 
	NodeRef	getParent() const { return mParent.lock(); }
	//! returns the node's parent node (provide a templated function for easier down-casting of nodes)
	template <class T>
	boost::shared_ptr<T>	getParent() const { 
		return boost::dynamic_pointer_cast<T>(mParent.lock()); 
	}
	//! returns a node higher up in the hierarchy of the desired type, if any
	template <class T>
	boost::shared_ptr<T>	getTreeParent() const { 
		boost::shared_ptr<T> node = boost::dynamic_pointer_cast<T>(mParent.lock()); 
		if(node) 
			return node;
		else if(mParent.lock()) 
			return mParent.lock()->getTreeParent<T>();
		else return node;
	}

	// functions to get the Node's unique identifier and to quickly find a Node with a specific uuid
	unsigned int	getUuid() const { return mUuid; }
	ci::Color		getUuidColor() const { return uuidToColor(mUuid); }

	static ci::Color	uuidToColor(unsigned int uuid){	return ci::Color((uuid & 0xFF) / 255.0f, ((uuid >> 8) & 0xFF) / 255.0f, ((uuid >> 16) & 0xFF) / 255.0f); }
	static unsigned int	colorToUuid(ci::Color color){ return colorToUuid( (unsigned char)(color.r * 255), (unsigned char)(color.g * 255), (unsigned char)(color.b * 255) ); }
	static unsigned int	colorToUuid(unsigned char r, unsigned char g, unsigned char b){ return r + (g << 8) + (b << 16); }

	static NodeRef		findNode(unsigned int uuid){ return uuidLookup[uuid].lock(); }

	// parent functions
	//! returns wether this node has a specific child
	bool hasChild(NodeRef node) const ;
	//! adds a child to this node if it wasn't already a child of this node
	void addChild(NodeRef node); 
	//! removes a specific child from this node
	void removeChild(NodeRef node);
	//! removes all children of this node
	void removeChildren();
	//! puts a specific child on top of all other children of this node
	void putOnTop(NodeRef node);
	//! returns wether a specific child is on top of all other children
	bool isOnTop(NodeConstRef node) const ;		
	//! puts a specific child below all other children of this node
	void moveToBottom(NodeRef node);

	//! returns a list of all children of the specified type
	template <class T>
	std::deque< boost::shared_ptr<T> > getChildren() {
		std::deque< boost::shared_ptr<T> > result;
		NodeList::iterator itr;
		for(itr=mChildren.begin(); itr!=mChildren.end(); ++itr) {
			boost::shared_ptr<T>  node = boost::dynamic_pointer_cast<T>(*itr); 
			if(node) result.push_back(node);
		}
		return result;
	}

	//!
	NodeRef findChild(unsigned int uuid);

	// child functions
	//! removes this node from its parent
	void removeFromParent();
	//! puts this node on top of all its siblings
	void putOnTop();
	//! returns wether this node is on top of all its siblings
	bool isOnTop() const ;		
	//! puts this node below all its siblings
	void moveToBottom();

	//! enables or disables visibility of this node (invisible nodes are not drawn and can not receive events, but they still receive updates)
	virtual void setVisible(bool visible=true){ mIsVisible = visible; }
	//! returns wether this node is visible
	virtual bool isVisible() const { return mIsVisible; }
	//!
	virtual bool toggleVisible(){ setVisible(!mIsVisible); return mIsVisible; }	

	//! 
	virtual void setClickable(bool clickable=true){ mIsClickable = clickable; }
	//! returns wether this node is clickable
	virtual bool isClickable() const { return mIsClickable; }

	//! returns the transformation matrix of this node
	const ci::Matrix44f& getTransform() const { if(mIsTransformInvalidated) transform(); return mTransform; }
	//! returns the accumulated transformation matrix of this node
	const ci::Matrix44f& getWorldTransform() const { if(mIsTransformInvalidated) transform(); return mWorldTransform; }
	//!
	void invalidateTransform() const { mIsTransformInvalidated = true; }

	//! 
	virtual void setSelected(bool selected=true){ mIsSelected = selected; }
	//! returns wether this node is selected
	virtual bool isSelected() const { return mIsSelected; }

	//! signal parent that this node has been clicked or activated
	virtual void selectChild(NodeRef node) {
		NodeList::iterator itr;
		for(itr=mChildren.begin(); itr!=mChildren.end(); ++itr) 
			(*itr)->setSelected( *itr == node );
	}
	//! signal parent that this node has been released or deactivated
	virtual void deselectChild(NodeRef node) {
		NodeList::iterator itr;
		for(itr=mChildren.begin(); itr!=mChildren.end(); ++itr) 
			(*itr)->setSelected( false );
	}

	// tree parse functions
	//! calls the setup() function of this node and all its decendants
	void treeSetup();
	//! calls the shutdown() function of this node and all its decendants
	void treeShutdown();
	//! calls the update() function of this node and all its decendants
	void treeUpdate(double elapsed=0.0);
	//! calls the draw() function of this node and all its decendants
	void treeDraw();

	virtual void setup() {}
	virtual void shutdown() {}
	virtual void update( double elapsed=0.0 ) {}
	virtual void draw() {}

	// supported events
	//! calls the mouseMove() function of this node and all its decendants until a TRUE is passed back
	bool treeMouseMove( ci::app::MouseEvent event );
	//! calls the mouseDown() function of this node and all its decendants until a TRUE is passed back
	bool treeMouseDown( ci::app::MouseEvent event );
	//! calls the mouseDrag() function of this node and all its decendants until a TRUE is passed back
	bool treeMouseDrag( ci::app::MouseEvent event );
	//! calls the mouseUp() function of this node and all its decendants until a TRUE is passed back
	bool treeMouseUp( ci::app::MouseEvent event );

	virtual bool mouseMove( ci::app::MouseEvent event );
	virtual bool mouseDown( ci::app::MouseEvent event );
	virtual bool mouseDrag( ci::app::MouseEvent event );
	virtual bool mouseUp( ci::app::MouseEvent event );

	// support for easy picking system
	virtual bool mouseUpOutside( ci::app::MouseEvent event );
	
	//! calls the keyDown() function of this node and all its decendants until a TRUE is passed back
	bool treeKeyDown( ci::app::KeyEvent event );
	//! calls the keyUp() function of this node and all its decendants until a TRUE is passed back
	bool treeKeyUp( ci::app::KeyEvent event );

	virtual bool keyDown( ci::app::KeyEvent event );
	virtual bool keyUp( ci::app::KeyEvent event );

	//! calls the resize() function of this node and all its decendants until a TRUE is passed back
	bool treeResize();

	virtual bool resize();

	// stream support
	virtual inline std::string toString() const { return "Node"; }
	friend std::ostream& operator<<(std::ostream& s, const Node& o){ return s << "[" << o.toString() << "]"; }
protected:
	bool					mIsVisible;
	bool					mIsClickable;
	bool					mIsSelected;

	const unsigned int		mUuid;

	NodeWeakRef				mParent;
	NodeList				mChildren;

	ci::ColorA				mColor;

	mutable bool			mIsTransformInvalidated;
	mutable ci::Matrix44f	mTransform;
	mutable ci::Matrix44f	mWorldTransform;
protected:
	//! helper function for coordinate transformations
	ci::Vec2f	project( float x, float y ) const ;
	ci::Vec2f	project( const ci::Vec2f &pt ) const { return project(pt.x, pt.y); }
	ci::Vec2f	project( const ci::Vec3f &pt ) const { return project(pt.x, pt.y); }

	ci::Vec3f	unproject( float x, float y, float z ) const ;
	ci::Vec3f	unproject( const ci::Vec2f &pt ) const { return unproject(pt.x, pt.y, 0.0f); }
	ci::Vec3f	unproject( const ci::Vec3f &pt ) const { return unproject(pt.x, pt.y, pt.z); }

	//! function that is called right before drawing this node
	virtual void predraw(){}
	//! function that is called right after drawing this node
	virtual void postdraw(){}

	//! required transform() function to populate the transform matrix
	virtual void transform() const = 0;
private:
	bool				mIsSetup;

	//! nodeCount is used to count the number of Node instances for debugging purposes
	static int			nodeCount;
	//! uuidCount is used to generate new unique id's
	static unsigned int uuidCount;
	//! uuidLookup allows us to quickly find a Node by id
	static NodeMap		uuidLookup;
};

// Basic support for OpenGL nodes
typedef boost::shared_ptr<class NodeGL> NodeGLRef;

class NodeGL :
	public Node
{
public:
	NodeGL(void){}
	virtual ~NodeGL(void){}	

	// shader support
	void	setShaderUniform( const std::string &name, int data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Vec2i &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const int *data, int count ){ if(mShader) mShader.uniform(name, data, count); }	
	void	setShaderUniform( const std::string &name, const ci::Vec2i *data, int count ){ if(mShader) mShader.uniform(name, data, count); }	
	void	setShaderUniform( const std::string &name, float data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Vec2f &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Vec3f &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Vec4f &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Color &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::ColorA &data ){ if(mShader) mShader.uniform(name, data); }
	void	setShaderUniform( const std::string &name, const ci::Matrix33f &data, bool transpose = false ){ if(mShader) mShader.uniform(name, data, transpose); }
	void	setShaderUniform( const std::string &name, const ci::Matrix44f &data, bool transpose = false ){ if(mShader) mShader.uniform(name, data, transpose); }
	void	setShaderUniform( const std::string &name, const float *data, int count ){ if(mShader) mShader.uniform(name, data, count); }
	void	setShaderUniform( const std::string &name, const ci::Vec2f *data, int count ){ if(mShader) mShader.uniform(name, data, count); }
	void	setShaderUniform( const std::string &name, const ci::Vec3f *data, int count ){ if(mShader) mShader.uniform(name, data, count); }
	void	setShaderUniform( const std::string &name, const ci::Vec4f *data, int count ){ if(mShader) mShader.uniform(name, data, count); }

	void	bindShader(){ if(mShader) mShader.bind(); }
	void	unbindShader(){ if(mShader) mShader.unbind(); }

	// stream support
	virtual inline std::string toString() const { return "NodeGL"; }
protected:
	ci::gl::GlslProg	mShader;
};

// Basic support for 2D nodes
typedef boost::shared_ptr<class Node2D> Node2DRef;

class Node2D :
	public NodeGL
{
public:
	Node2D(void);
	virtual ~Node2D(void);

	// getters and setters
	virtual ci::Vec2f	getPosition() const { return mPosition; }
	virtual void		setPosition( float x, float y ){ mPosition = ci::Vec2f(x, y); invalidateTransform(); }
	virtual void		setPosition( const ci::Vec2f &pt ){ mPosition = pt; invalidateTransform(); }

	virtual ci::Quatf	getRotation() const { return mRotation; }
	virtual void		setRotation( float radians ){ mRotation.set( ci::Vec3f::zAxis(), radians ); invalidateTransform(); }
	virtual void		setRotation( const ci::Quatf &rot ){ mRotation = rot; invalidateTransform(); }

	virtual ci::Vec2f	getScale() const { return mScale; }
	virtual void		setScale( float scale ){ mScale = ci::Vec2f(scale, scale); invalidateTransform(); }
	virtual void		setScale( float x, float y ){ mScale = ci::Vec2f(x, y); invalidateTransform(); }
	virtual void		setScale( const ci::Vec2f &scale ){ mScale = scale; invalidateTransform(); }

	virtual ci::Vec2f	getAnchor() const { return mAnchorIsPercentage ? mAnchor * getSize() : mAnchor; }
	virtual void		setAnchor( float x, float y ){ mAnchor = ci::Vec2f(x, y); mAnchorIsPercentage = false; invalidateTransform(); }
	virtual void		setAnchor( const ci::Vec2f &pt ){ mAnchor = pt; mAnchorIsPercentage = false; invalidateTransform(); }

	virtual	ci::Vec2f	getAnchorPercentage() const { return mAnchorIsPercentage ? mAnchor : mAnchor / getSize(); }
	virtual	void		setAnchorPercentage(float px, float py){ mAnchor = ci::Vec2f(px, py); mAnchorIsPercentage = true; invalidateTransform(); }
	virtual	void		setAnchorPercentage(const ci::Vec2f &pt){ mAnchor = pt; mAnchorIsPercentage = true; invalidateTransform(); }

	// 
	virtual float		getWidth() const { return mWidth; }
	virtual float		getScaledWidth() const { return mWidth * mScale.x; }
	virtual float		getHeight() const { return mHeight; }
	virtual float		getScaledHeight() const { return mHeight * mScale.y; }
	virtual ci::Vec2f	getSize() const { return ci::Vec2f(mWidth, mHeight); }
	virtual ci::Vec2f	getScaledSize() const { return getSize() * mScale; }
	virtual ci::Rectf	getBounds() const { return ci::Rectf(ci::Vec2f::zero(), getSize()); }
	virtual ci::Rectf	getScaledBounds() const { return ci::Rectf(ci::Vec2f::zero(), getScaledSize()); }

	virtual void		setWidth(float w){ mWidth=w; }
	virtual void		setHeight(float h){ mHeight=h; }
	virtual void		setSize(float w, float h){ mWidth=w; mHeight=h; }
	virtual void		setSize( const ci::Vec2i &size ){ mWidth=(float)size.x; mHeight=(float)size.y; }
	virtual void		setBounds( const ci::Rectf &bounds ){ mWidth=bounds.getWidth(); mHeight=bounds.getHeight(); }

	// conversions from screen to world to object coordinates and vice versa
	virtual ci::Vec2f screenToParent( const ci::Vec2f &pt ) const ;
	virtual ci::Vec2f screenToObject( const ci::Vec2f &pt ) const ;
	virtual ci::Vec2f parentToScreen( const ci::Vec2f &pt ) const ;
	virtual ci::Vec2f parentToObject( const ci::Vec2f &pt ) const ;
	virtual ci::Vec2f objectToParent( const ci::Vec2f &pt ) const ;
	virtual ci::Vec2f objectToScreen( const ci::Vec2f &pt ) const ;

	// stream support
	virtual inline std::string toString() const { return "Node2D"; }
protected: 
	ci::Vec2f	mPosition;
	ci::Quatf	mRotation;
	ci::Vec2f	mScale;
	ci::Vec2f	mAnchor;

	bool		mAnchorIsPercentage;

	float		mWidth;
	float		mHeight;

	// required function (see: class Node)
	virtual void transform() const {
		// construct transformation matrix
		mTransform.setToIdentity();
		mTransform.translate( ci::Vec3f( mPosition, 0.0f ) );
		mTransform *= mRotation.toMatrix44();
		mTransform.scale( ci::Vec3f( mScale, 1.0f ) );

		if( mAnchorIsPercentage )
			mTransform.translate( ci::Vec3f( -mAnchor * getSize(), 0.0f ) );
		else
			mTransform.translate( ci::Vec3f( -mAnchor, 0.0f ) );

		// update world matrix (TODO will not work with cached matrix!)
		Node2DRef parent = getParent<Node2D>();
		if(parent)
			mWorldTransform = parent->mWorldTransform * mTransform;
		else mWorldTransform = mTransform;

		// TODO set mIsTransformValidated to false once the world matrix stuff has been rewritten
	}
};

// Basic support for 3D nodes
typedef boost::shared_ptr<class Node3D> Node3DRef;

class Node3D :
	public NodeGL
{
public:
	Node3D(void);
	virtual ~Node3D(void);

	//! the drawMesh function only draws a mesh without binding textures and shaders
	virtual void		drawWireframe(){}
	virtual void		treeDrawWireframe();

	// getters and setters
	virtual ci::Vec3f	getPosition() const { return mPosition; }
	virtual void		setPosition( float x, float y, float z ){ mPosition = ci::Vec3f(x, y, z); invalidateTransform(); }
	virtual void		setPosition( const ci::Vec3f &pt ){ mPosition = pt; invalidateTransform(); }

	virtual ci::Quatf	getRotation() const { return mRotation; }
	virtual void		setRotation( float radians ){ mRotation.set( ci::Vec3f::yAxis(), radians ); invalidateTransform(); }
	virtual void		setRotation( const ci::Vec3f &radians ){ mRotation.set( radians.x, radians.y, radians.z ); invalidateTransform(); }
	virtual void		setRotation( const ci::Vec3f &axis, float radians ){ mRotation.set( axis, radians ); invalidateTransform(); }
	virtual void		setRotation( const ci::Quatf &rot ){ mRotation = rot; invalidateTransform(); }

	virtual ci::Vec3f	getScale() const { return mScale; }
	virtual void		setScale( float scale ){ mScale = ci::Vec3f(scale, scale, scale); invalidateTransform(); }
	virtual void		setScale( float x, float y, float z ){ mScale = ci::Vec3f(x, y, z); invalidateTransform(); }
	virtual void		setScale( const ci::Vec3f &scale ){ mScale = scale; invalidateTransform(); }

	virtual ci::Vec3f	getAnchor() const { return mAnchor; }
	virtual void		setAnchor( float x, float y, float z ){ mAnchor = ci::Vec3f(x, y, z); invalidateTransform(); }
	virtual void		setAnchor( const ci::Vec3f &pt ){ mAnchor = pt; invalidateTransform(); }

	// stream support
	virtual inline std::string toString() const { return "Node3D"; }
protected: 
	ci::Vec3f	mPosition;
	ci::Quatf	mRotation;
	ci::Vec3f	mScale;
	ci::Vec3f	mAnchor;

	ci::gl::Material	mMaterial;

	// required function (see: class Node)
	virtual void transform() const {
		// construct transformation matrix
		mTransform.setToIdentity();
		mTransform.translate( mPosition );
		mTransform *= mRotation.toMatrix44();
		mTransform.scale( mScale );
		mTransform.translate( -mAnchor );

		// update world matrix
		Node3DRef parent = getParent<Node3D>();
		if(parent)
			mWorldTransform = parent->mWorldTransform * mTransform;
		else mWorldTransform = mTransform;
	}
};

} } // namespace ph::nodes