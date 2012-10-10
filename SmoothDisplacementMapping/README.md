Smooth Displacement Mapping
===========================


Vertex displacement mapping is a technique where you use a texture or a procedural function to change the position of vertices in a mesh. It is often used to create a terrain with mountains from a height field. 


But when the vertex position changes, its associated normal vector should also change, because the mesh is no longer flat. One way to calculate the new normal would be by using a geometry shader. This is often called per-face normal computation on the GPU, because it can only calculate one normal vector for the whole triangle. The result looks facetted: each triangle will appear to be flat.


To overcome this, you should calculate a new normal map if your height field (a.k.a. displacement map) changes. This can easily be done on the GPU, which is great if your displacement map changes every frame.


This sample will show you how to:
* render to a floating point texture to create a displacement map on-the-fly
* render the corresponding normal map, also a floating point texture
* use both map to render a animated, transparent piece of cloth that looks a lot like the PlayStation's XrossMediaBar background (but that's just a coincidence, don't you think? ;))


<u>Controls:</u>
* use the <b>mouse</b> to control the camera
* press <b>SPACE</b> to reset the camera
* press <b>M</b> to toggle the undisplaced mesh
* press <b>W</b> to toggle wire frame rendering
* press <b>T</b> to toggle texture display
* press <b>V</b> to toggle vertical sync
* press <b>F</b> to toggle full screen
* press <b>ESC</b> to quit


-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


