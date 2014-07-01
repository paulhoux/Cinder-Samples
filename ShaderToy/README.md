ShaderToy
=========

This is a WINDOWS ONLY sample.

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/ShaderToy/PREVIEW.png)

The purpose of this sample is to show how you can create a shared OpenGL context to load and compile shaders in a second thread. This will allow you to transition (almost) seamlessly from one shader to another. The same technique also works with textures, but this is not part of the sample.

To showcase this technique, the sample uses shader code taken from the excellent ShaderToy website:
https://www.shadertoy.com

All of the amazing shaders were done by the incredible Iñigo Quilez:
https://www.shadertoy.com/user/iq

Just like the website, you can use the mouse to control the shader. Press SPACE to load another, random shader. Press SPACE several times to queue shaders - they will be loaded one after the other. Press ESC to quit. You can drag&drop fragment files from the assets folder to the window to load them.

If you want to try other shaders from that website, it should be sufficient to copy-paste the unaltered shader code to a *.frag file of its own. You may need to change the textures assigned to the 4 texture slots for the shader to work, as they are hard-coded at the moment. The shaders supplied with this sample all use the same textures.


Copyright (c) 2014, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
