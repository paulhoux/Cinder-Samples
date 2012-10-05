Stars
=============

Note: at the moment this sample needs code from several commits to the Cinder framework in order to compile successfully. To compile, check out the 'paulhoux:unicode_linebreak' branch on the Cinder repository and recompile Cinder. In the near future, this will hopefully be made easier.


Detailed GIT instructions:

1. I assume you have cloned Cinder to a folder named "cinder_master" and my Cinder-Samples to a folder named "cinder_samples", as described in {root}/README.md.
2. Open a command window and switch to the drive that contains your "cinder_master" folder: ```>d:```
3. Go to the "cinder_master" directory: ```>cd "\path\to\cinder_master"```
4. Check if 'paulhoux' is added as a remote: ```>git remote show```
5. If not, add it: ```>git remote add paulhoux https://github.com/paulhoux/Cinder.git```
6. Fetch the paulhoux repository: ```>git fetch paulhoux```
7. If you have made local changes to the repository, commit or revert them.
8. Finally, checkout the 'paulhoux:unicode_linebreak' branch: ```>git checkout -b paulhoux/unicode_linebreak```
9. Recompile Cinder.


This sample will show you how to:
* render advanced, animated point sprites using shaders
* construct VBO meshes that are not GL_TRIANGLES like TriMesh
* use the IrrKlang sound engine to play sound effects and music
* use the new CameraStereo class to render in stereoscopic 3D (side-by-side)
* create your own camera class that can be animated or controlled by the user
* read and parse a text file containing data
* read and write a binary data file for faster loading


Note: for the sample to play music, add MP3, WAV, OGG and/or FLAC files to the <i>./assets/music</i> folder. 


<u>Controls:</u>
* use the <b>mouse</b> to control the camera
* press <b>SPACE</b> to enable automatic camera animation
* press <b>S</b> to toggle stereoscopic (side-by-side) 3D
* press <b>G</b> to toggle the celestial grid
* press <b>L</b> to toggle name labels
* press <b>C</b> to toggle constellations
* press <b>V</b> to toggle vertical sync
* press <b>F</b> to toggle full screen
* press <b>A</b> to show/hide the cursor arrow
* press <b>ESC</b> to quit
* press <b>MEDIA_NEXT_TRACK</b> to play the next song
* press <b>MEDIA_PREV_TRACK</b> to play the previous song
* press the <b>MEDIA_STOP</b> or <b>MEDIA_PLAY_PAUSE</b> keys to stop, play and pause the music

-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


