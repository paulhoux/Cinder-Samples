Post Processing
===============

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/PostProcessing/PREVIEW.png)

This sample will show you how to:
* load and display an image file
* load, play and display a QuickTime movie
* load and compile a GLSL shader
* perform real-time post-processing on the image or movie


The post-processing will make your image or movie look as if it was projected using an old CRT-projector. The effects applied to the images are:
* color separation (red, green and blue don't line up)
* increased contrast 
* vignetting (colors are darker at the edges)
* tinting (colors become slightly more green)
* TV lines (horizontal lines, like a television)
* flickering (brightness changes over time)


All kudo's for the shader should go to Iñigo Quílez (http://www.iquilezles.org/). See the ShaderToy in the "Lemon" section of his website.


For more information, see: http://forum.libcinder.org/topic/problems-loading-glsl-from-disk-resource#23286000000775003


<u>Controls:</u>
* press <b>F</b> to toggle full screen
* press <b>ESC</b> to quit

-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


