Audio Visualizer
================

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/AudioVisualizer/PREVIEW.png)


This sample shows how to play audio using Cinder's FMOD block. The audio's spectrum is then calculated and rendered as a scrolling height field.

Audio is played using Cinder's own FMOD block. Retrieving the FFT spectrum data is fairly easy with a simple call to ```mFMODSystem->getSpectrum()``` for the left and right audio channel. The data is returned as floats, ranging from 0.0 to 1.0. This data is then stored in a single row of a ```Channel32f```, from which we can easily create an OpenGL texture.

A static mesh is created that will be deformed by the spectrum textures. All the animation is done in shaders. The vertex shader averages the data from the left and right channel and converts it to decibels. The resulting value is then used to push vertices up along the y-axis, effectively creating a height field.

By offsetting the texture coordinates, we can make sure the most recently captured spectrum is always at the edge of the mesh. OpenGL will automatically wrap the texture, because we have set the mode to ```GL_REPEAT```, so it's taking care of the scrolling and we don't need to do the hard work.

Finally, the fragment shader will create rainbow colored line strips, based on the supplied interpolated vertex color and the texture coordinates. Rendering is done using additive blending, for a nice glowing neon effect.


Copyright (c) 2014, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
