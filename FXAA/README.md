FXAA
====

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/FXAA/PREVIEW.png)

This sample implements FXAA, a technique to reduce aliasing effects using a post-processing step. Normally, we render to a multi-sampled buffer to reduce jagged edges, but this can be very costly in terms of memory and performance. Sometimes (e.g. when doing deferred shading) it isn't even possible. For those cases, FXAA can be applied to anti-alias the final image using a post-process filter.

In order for this to work, FXAA should be applied after rendering and tone mapping. The input color should be non-linear (e.g. gamma corrected) and for best results the luminance of the pixel should be stored in the alpha channel. 

When running the sample, you can drag the divider to examine the effect of FXAA. Notice how sharp edges become noticeably smoother. You can freeze time by pressing the space key.


Copyright (c) 2014, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
