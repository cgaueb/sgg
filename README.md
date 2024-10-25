# Simple Game Graphics Library

This library (SGG) aims to provide a simple interface for students to include basic graphics, audio and interaction functionality in their applications written in C++. SGG offers an abstraction and simplification layer over SDL, OpenGL and other core windowing, graphics and audio libraries, providing a clear, simple and hasle - free programming interface to build from simple interactive applications to 2D computer games and data visualization tools.
- Original Repo can be found [here](https://github.com/cgaueb/sgg)
- Documentation can be found [here](https://cgaueb.github.io/sgg/index.html "SGG's Documentation")

## Installation (Windows x64 Exclusively)

- You need the MVSC compiler, trying to compile with MinGW via CMake won't work, it won't recognise C++ libraries even with vcpkg config.
- Run _build_sgg_x64.bat_ and wait for the compiler to finish.
-  - When you compile the SGG your library should be under ./lib/. You need sgg.lib and  sggd.lib.

```bat
Dunno i just use CLion and you should too. Fuck Visual Studio...
```

## Contributors
 31/08/2021 - [Liarokapis Alexandros](https://github.com/liarokapisv "Liarokapis Alexandros") (CMake build scripts and instructions)
 
 19/11/2020 - [Georgios E. Syros](https://github.com/gsiros "Georgios E. Syros") (Mac OS X build scripts and instructions)
 
