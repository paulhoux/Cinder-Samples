Simple Scene Graph
==================

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/SimpleSceneGraph/PREVIEW.png)

This sample will show you how to:
* create a scene made of related objects
* detect which object was clicked
* extend the Node class to create custom objects


I tried to live without a scene graph and it gave me severe headaches and suicidal tendencies. That's why at one point in my project I decided to throw much of what I had done out of the window and create my own scene graph.


First, I looked at OpenScenegraph, to see if I could use it with Cinder. Unfortunately (for us), it wants to take control of much of the render pipeline, so it isn't easy (if even possible) to let OSG and Cinder live next to each other. I did, however, borrow some ideas from their implementation. But for the most part, my implementation is radically different. For instance, OSG uses special transform nodes, that sit between objects. I chose to incorporate transforms into the nodes themselves. While OSG's system is more flexible and optimized, mine is a lot easier to setup and use from code.


Another alternative scene graph would be FBX, which I will probably start using instead of my own scene graph as soon as I have learned more about it. It supports animation and skinning, among other things, and is backed by great 3D modelling packages. It will allow you to work together with a modeler and animator, instead of having to hard-code all your animations.


As I said, my scene graph doesn't come with documentation, but here are a few tips:
* To create your objects, extend either the Node2D or Node3D class.
* Your class should then implement the basic setup(), update(double elapsed) and draw() methods and you are good to go.
* In your main application's setup(), create a root Node3D for your 3D world and a root Node2D for your 2D interface.
* Create the rest of the nodes and add them as children to the root or to each other. Each node can only be a child of one parent node, but a node can have as many children as you like.
* Node2D can be a child of Node3D and the other way around. In most cases, however, it doesn't make much sense to mix and match them and converting mouse coordinates to object space may not give the right results.
* When all children have been created, make sure to call root->treeSetup() to recursively call each node's setup() function. This will be done from 'trunk' to 'leaf', so a node's parent will be initialized before the node itself is initialized, which is the right way to do things.
* In your main application's update(), calculate the elapsed time in seconds and then call root->treeUpdate(elapsed) to recursively call each node's update() function (trunk to leaf).
* In your main application's draw(), setup the camera any way you like, then call root->treeDraw().
* A node's predraw() function is called just before it is drawn. Use it to setup render states for the whole tree of which this node is the trunk. For example, you can bind an FBO so everything will be drawn to the FBO instead. Clean up after yourself in the postdraw() function.
* To pass a MouseEvent to your nodes, simply call e.g. root->treeMouseDown(event). The event will be processed from 'leaf' to 'trunk', so everything on top will be checked before going deeper into your scene. Mouse position is passed as screen coordinates, so you may have to use conversion methods like screenToObject() to convert to object space. Note that root->treeMouseMove(event) is usually too slow - tree traversal is not optimized in my system and in this particular case most nodes will not react to the event so it has to visit a lot of nodes before being handled. 


Although my scene graph is far from perfect, in practice it does its job and helps me keep my sanity :)


-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


