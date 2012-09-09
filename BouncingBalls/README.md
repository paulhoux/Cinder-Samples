BouncingBalls
=============

This sample will show you how to:
* create a fast-drawing, textured circle on the fly using a VboMesh
* apply simple physics (velocity, gravity) to animate the ball
* bounce the ball off of walls
* check each pair of balls for collision efficiently
* collide the ball with other balls using 2D vector projection
* create a simple version of motion blur for smoother animation using additive blending
* run the simulation a fixed number of steps per second to be frame rate independent
* use a Timer to easily pause and resume the simulation


You can add or remove balls using the PLUS and MINUS keys. Note how the balls bounce off the walls and collide with each other.

Freeze the simulation by pressing RETURN and note the motion blur effect.

Toggle motion blur using the M key, then resume the simulation and note how the balls now seem to have an annoying stippled trail (stroboscope effect) that wasn't visible when motion blur was active.

Reduce the frame rate using the 1, 2, 3 and 4 keys and note how the simulation is still running at the same speed, but the motion blur trails are now much longer (the 'shutter' of the camera is now open a lot longer).


If you add a lot of balls, you will notice that they will not come to a full rest. This is mainly due to the fact that we are not properly calculating the forces on each ball, but immediately try to set the velocity based on gravity and collisions. 


-Paul


Copyright (c) 2012, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


