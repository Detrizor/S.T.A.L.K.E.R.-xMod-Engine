## About
This repository contains engine source code of S.T.A.L.K.E.R. xMod, which is based on an outdated version (from 22.03.2018) of Call of Chernobyl modification's engine source code, which itself is based on S.T.A.L.K.E.R. Call of Pripyat's code version 1.6.02.
The original game is released by GSC Game World and any changes to it's engine are allowed for ***non-commercial*** use only (see [License.txt] for details).

The goal of xMod project is to expand, reshape and rebuild the features of the original game and those added by various modifications. General concept of in-game world and lore is seriously shifted towards S.T.A.L.K.E.R.'s book lore, realism and specific author's vision.

## Build tips
* This project is maintained under Visual Studio 2013 (untested branch is already transfered to VS2022, soon will be merged to master).
* If you want to place your compiled binaries straight to the game folder, then open Property Manager tab (View > Other Windows > Property Manager) and under User Macros change xrGameDir to point to the game root folder. 
* lua51.dll is being built into `src\3rd party\luajit-2\bin\x86`, not primary binaries output location (xrGameDir).
* Currently only Release/Win32 will build.
