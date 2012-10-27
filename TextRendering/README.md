Text Rendering using Signed Distance Fields
===========================================

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/TextRendering/PREVIEW.png)

Note: at the moment this sample needs code from several commits to the Cinder framework in order to compile successfully. To compile, check out the 'paulhoux:unicode_linebreak' branch on the Cinder repository and recompile Cinder. In the near future, this will hopefully be made easier.


Detailed GIT instructions:

1. I assume you have cloned Cinder to a folder named "cinder_master". See for more information: http://libcinder.org/docs/welcome/GitSetup.html
2. Open a command window and switch to the drive that contains your "cinder_master" folder: ```d:```
3. Go to the "cinder_master" directory: ```cd "\path\to\cinder_master"```
4. Check if 'paulhoux' is added as a remote: ```git remote show```
5. If not, add it: ```git remote add paulhoux https://github.com/paulhoux/Cinder.git```
6. Fetch the paulhoux repository: ```git fetch paulhoux```
7. If you have made local changes to the repository, commit or revert them.
8. Finally, checkout the 'paulhoux:unicode_linebreak' branch: ```git checkout -b paulhoux/unicode_linebreak```
9. Recompile Cinder.


This sample is a WORK IN PROGRESS and actually not meant as a sample, but as a test application for ultra-fast text rendering in OpenGL using Signed Distance Fields. This technique, pioneered by Valve in 2007, allows scaling bitmap fonts without much loss in visual quality. And since bitmap fonts are extremely fast to render and 100% cross-platform, I personally can't think of a better way to render text.


On the TODO list: text markup, text on path, justified text, threaded text boxes, support for more than 1 font, font style, font color or font size in a single text.


Run the sample for more information.


-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


