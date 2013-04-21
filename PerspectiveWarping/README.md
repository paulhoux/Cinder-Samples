Perspective Warping
===================

![Preview](https://raw.github.com/paulhoux/Cinder-Samples/master/PerspectiveWarping/PREVIEW.png)


This sample will show you how to perform perspective warping of an image using OpenCV. This technique is required if you want to correctly project an image onto a wall using a beamer that is not perfectly aligned. 


Perspective warping is similar to, but not the same as, keystone correction. Keystone correction is often limited to either horizontal or vertical correction, which will still require you to setup the beamer perfectly centered to the screen.


In contrast, perspective warping allows you to move each individual corner of your content to a specific point on the projection surface, while maintaining a perspectively correct image. Straight lines in your content remain straight.


Calculating the warp parameters requires a non-trivial algorithm. Luckily for us, the OpenCV library can do it for us. OpenCV comes with the release version of Cinder. If you, instead, use the GitHub version of Cinder, you'll have to add the Cinder-OpenCV block yourself:

1. Open a command window and switch to the drive that contains your "cinder_master" folder: ```d:```
2. Go to the "cinder_master" directory: ```cd "\path\to\cinder_master"```
3. Go to the "blocks" directory: ```cd blocks```
4. Clone the OpenCV block: ```git clone https://github.com/cinder/Cinder-OpenCV.git opencv```


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.