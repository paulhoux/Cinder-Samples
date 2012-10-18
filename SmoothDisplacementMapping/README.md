Smooth Displacement Mapping
===========================

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/SmoothDisplacementMapping/PREVIEW.png)


Vertex displacement mapping is a technique where a texture or a procedural function is used to dynamically change the position of vertices in a mesh. It is often used to create a terrain with mountains from a height field, ocean waves or an animated flag. 


For dynamically changing height fields, like an animated flag, not only the vertex position should change, but also its associated normal vector. One way to calculate the new normal would be to use a geometry shader. This is often called per-face normal computation on the GPU, because it can only calculate one normal vector for the whole triangle. The result looks faceted: each triangle will appear to be flat.


To overcome this, you can use a normal map. This is an additional texture that corresponds to the height field and contains offsets to the original surface normals. A shader can then fetch the normal from this texture on a per pixel basis. The great news is that normals are even automatically interpolated, so per-pixel shading will look incredibly smooth. 


You should calculate a new normal map every time your height field (a.k.a. displacement map) changes. This can easily be done on the GPU. Using floating point textures, this is even easier and can be done using a simple fragment shader.


This sample will show you how to:
* render to a floating point texture to create a displacement map on-the-fly
* render the corresponding normal map, also a floating point texture
* use both maps to render an animated, transparent piece of cloth that looks a lot like Sony PlayStation's XrossMediaBar background (but that's just a coincidence, don't you think? ;))


All of this is done on the GPU, very little work remains for the CPU. The sample can therefor easily run at 300+ FPS. Note that you do need a modern GPU for this, with support for floating point textures and vertex shader texture fetch. If your GPU supports shader model 3, you're probably good to go.


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


