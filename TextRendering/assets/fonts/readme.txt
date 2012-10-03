This text is rendered very efficiently using a special Signed Distance Field font file (SDFF) and can be scaled to any size without much loss in rendering quality.

Press [ or ] to change font size.
Press LEFT or RIGHT to change alignment.
Press UP or DOWN to change line spacing.
Press SPACE to swap colors.
Press F to toggle full screen.
Press V to toggle vertical sync.
Press W to toggle wire frames.
Press ESC to quit.

Tip: drag a single TXT file or another SDFF font file onto the TextRendering window to load and display it.

To create a new cross-platform SDFF font file:

1) Drag a font file (.TTF, .OTF) onto the SDFont.exe application, which you can find in "./assets/fonts"
2) Specify a texture size, e.g. 1024
3) Set the maximum character value (usually 65535)
4) The ideal font pixel size should be around 50 or bigger (if not, change the texture size).
5) The SDFont.exe will generate two files: a TXT and a PNG.
6) Optionally edit the font name in the TXT file. This might be necessary if you are using fonts of the same family but with different weights or styles (italic, bold). The SDFont.exe application might give them all the same name.
7) Run the TextRendering sample and drag the resulting PNG and TXT files (both together) onto the window.
8) Enjoy your new SDFF font file! It will be stored in "./assets/fonts"

©2010-2012 Paul Houx - All Rights Reserved

