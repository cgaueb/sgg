# Simple Game Graphics Library

This library (SGG) aims to provide a simple interface for students to include basic graphics, audio and interaction functionality in their applications written in C and C++.SGG offers an abstraction and simplification layer over SDL, OpenGL and other core windowing, graphics and audio libraries, providing a clear, simple and hasle - free programming interface to build from simple interactive applications to 2D computer games and data visualization tools.

Documentation can be found [here](https://cgaueb.github.io/sgg/index.html "SGG's Documentation")

## Contributors

 19/11/2020 - [Georgios E. Syros](https://github.com/gsiros "Georgios E. Syros") (Mac OS X build scripts and instructions)
 
 
## Building, Installation and Usage

### Installing vcpkg dependencies

```
vcpkg install glew:x64-windows-static
vcpkg install sdl2:x64-windows-static
vcpkg install sdl2-mixer:x64-windows-static
vcpkg install freetype:x64-windows-static
vcpkg install glm:x64-windows-static
```

### [Option 1 (Recommended)] Configuring project with vcpkg toolchain and registry support

This will automatically register the library's build tree to cmake.
It will also auto find it's dependencies through vcpkg's toolchain without requiring the
dependent project to use the vcpkg toolchain itself. 
This is the most convenient option when building for personal usage. 

```
cmake -B build -DCMAKE_BUILD_TYPE=Release \
                -DCMAKE_EXPORT_PACKAGE_REGISTRY=ON \
                -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALL_DIR/scripts/buildsystems/vcpkg.cmake \
                -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

### [Option 2] Configuring project with vcpkg toolchain and no registry support

This will force dependent projects to have a global or local installation of the package available.
The installed folder is supposed to be packaged and distributed to different computers so it can't reuse
the vcpkg config files as they may not be available. The dependent projects will have to either supply
the vcpkg toolchain themselves or use any other way so as to make the library's dependencies discoverable. For example one may
also use the conan package manager, cmake-supplied Find modules, system installed config files or local Find modules.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release \
               -DCMAKE_TOOLCHAIN_FILE=$VCPKG_INSTALL_DIR/scripts/buildsystems/vcpkg.cmake \
               -DVCPKG_TARGET_TRIPLET=x64-windows-static
```

### Building 

`cmake --build build`

At this point, if configured using Option 1, the library will be available for consumption from other cmake projects.

### Installation 

This is only relevant if going with Option 2. Mostly useful for proper packaging.

For global installation use:

`sudo cmake --build build --target install`

For installation to a destination folder, re-configure to set the `CMAKE_INSTALL_PREFIX`:

```
cmake -B build -DCMAKE_INSTALL_PREFIX=dest
cmake --build build --target install
```

## Usage

Dependent projects can use `find_package(sgg REQUIRED)` from their CMakeLists.txt and 
then link to the `sgg::sgg` target. 
