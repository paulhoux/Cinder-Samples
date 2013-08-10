Cinder-Samples
==============

Sample applications for the Cinder framework. 

These code samples are compatible with the Microsoft Visual C++ (Express) 2010 and 2012 compilers. Use the project files inside the ```vc10``` folders for Visual C++ 2010. For Visual C++ 2012 you should use the files inside the ```vc11``` folders. XCode support for MacOS X is not included, because I do not own an Apple and would not be able to update the project files.

The C++ and GLSL source code, however, should run on all platforms without modifications. I assume you run the samples on recent hardware, supporting shader model 4. I have tested the samples on NVIDIA GPU's, it might be possible that ATI GPU's may produce shader compilation errors. If you find an inconsistency or platform related bug, please notify me or create a pull request, so that I can fix the issues and make these samples as cross-platform as possible. 

Find specific, sample related information in the sample's README.md files.


#####Downloading and using the samples with Cinder
These samples are meant to be used with version 0.8.5 (or higher) of Cinder. Download the samples and place them next to Cinder's master folder:

![Folders](https://raw.github.com/paulhoux/Cinder-Samples/master/FOLDERS.jpg)

It is recommended to download these samples as a Git repository. Install a Git client (see also: http://libcinder.org/docs/welcome/GitSetup.html) and then do the following:
* Open a command window (or Terminal)
* Switch to the disk containing the Cinder root folder, e.g.: ```d:```
* Browse to the Cinder root folder: ```cd path\to\cinder_master```
* The samples must be placed next to the cinder_master folder, so go up one level: ```cd ..```
* Clone the samples repository: ```git clone https://github.com/paulhoux/Cinder-Samples.git cinder_samples```

Alternatively, you can download the repository as a [ZIP-file](https://github.com/paulhoux/Cinder-Samples/zipball/master) and manually add the files to a "cinder_samples" folder.


Make sure the Cinder master folder is called ```cinder_master```, so that the samples can find it automatically. To learn more about how to create a copy of the Cinder Github repository, visit: http://libcinder.org/docs/welcome/GitSetup.html 


Thanks to all contributors and to the people behind the Cinder framework for doing an excellent job!

-Paul


Copyright (c) 2012-2013, Paul Houx - All rights reserved. This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


