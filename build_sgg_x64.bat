@SETLOCAL
@echo off

set STUDIO_VERSION=None
set STUDIO_EDITION=None
set STUDIO_PATH=C:\Program Files (x86)\Microsoft Visual Studio
set STUDIO_PATH_X64=C:\Program Files\Microsoft Visual Studio

IF EXIST "%STUDIO_PATH%\2017\" (
set STUDIO_VERSION=2017
)
IF EXIST "%STUDIO_PATH%\2019\" (
set STUDIO_VERSION=2019
)
IF EXIST "%STUDIO_PATH%\2022\" (
set STUDIO_VERSION=2022
)
IF EXIST "%STUDIO_PATH_X64%\2022\" (
set STUDIO_VERSION=2022
set STUDIO_PATH=C:\Program Files\Microsoft Visual Studio
)
IF EXIST "%STUDIO_PATH%\%STUDIO_VERSION%\Enterprise" (
set STUDIO_EDITION=Enterprise
)
IF EXIST "%STUDIO_PATH%\%STUDIO_VERSION%\Professional" (
set STUDIO_EDITION=Professional
)
IF EXIST "%STUDIO_PATH%\%STUDIO_VERSION%\Community" (
set STUDIO_EDITION=Community
)

IF %STUDIO_VERSION% == None (
    echo "Wrong Visual Studio Version Installed (2017, 2019 or 2022 required)"
    EXIT /B 1
)

IF %STUDIO_EDITION% == None (
    echo "Wrong Visual Studio Edition Installed (Community Edition is required)"
    EXIT /b 1
)

echo Compiling with Visual Studio %STUDIO_VERSION% %STUDIO_EDITION%

call "%STUDIO_PATH%\%STUDIO_VERSION%\%STUDIO_EDITION%\VC\Auxiliary\Build\vcvars64.bat"

set INCLUDE_PATH=.
set SGG_HEADERS_PATH=sgg/headers
set THIRD_PARTY_INCLUDE_PATH=3rdparty/include
set BUILD_PATH=build\Release
set BUILD_PATH_DEBUG=build\Debug
set OUT_PATH=lib
set LIB_NAME=sgg
set LIB_NAME_DEBUG=sggd
set THIRD_PARTY_LIB_PATH=3rdparty/lib
REM Use MSVC subfolder for 64-bit libs
set THIRD_PARTY_LIB_PATH_MSVC=%THIRD_PARTY_LIB_PATH%/MSVC
set LIBS=SDL2_mixer.lib glew32s.lib SDL2.lib SDL2main.lib opengl32.lib freetype.lib
set COMPILER_OPTIONS=/c /EHsc /nologo
set DEBUG_MODE=/Zi /Od /DEBUG /D"_DEBUG" /MDd /std:c++17
set RELEASE_MODE=/O2 /MD /std:c++17
set LIB_OPTIONS=/LIBPATH:"%THIRD_PARTY_LIB_PATH_MSVC%" /SUBSYSTEM:CONSOLE /MACHINE:X64 /ignore:4006 /nologo

echo Creating directories
IF NOT EXIST build mkdir build
IF NOT EXIST %OUT_PATH% mkdir %OUT_PATH%
IF NOT EXIST %BUILD_PATH% mkdir %BUILD_PATH%
IF NOT EXIST %BUILD_PATH_DEBUG% mkdir %BUILD_PATH_DEBUG%
echo on

REM ========================= NEW CMAKE/NINJA BUILD =========================
REM Prefer modern CMake-based build; falls back to legacy compile if CMake not found.

where cmake >nul 2>&1
IF %ERRORLEVEL%==0 (
    echo "CMake detected – generating build files."

    REM Create build directory
    IF NOT EXIST build mkdir build

    REM Configure for x64 using the selected VS version toolchain
    cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release || goto :cmake_fail

    REM Build Release
    cmake --build build --config Release || goto :cmake_fail

    REM Build Debug (optional)
    cmake --build build --config Debug || echo "Debug build skipped (see above if errors)."

    REM Copy assets next to the executable if not already handled by CMake
    xcopy /E /I /Y assets build\bin\assets >nul 2>&1

    echo "CMake build completed successfully!"
    goto :eof
)

:cmake_fail
echo "CMake build failed – falling back to legacy batch compiler."

REM ========================= LEGACY MSVC BUILD =========================

@echo Compiling Release Build (legacy)
for /R %%f in (sgg\*.cpp) do (
    cl %COMPILER_OPTIONS% %RELEASE_MODE% /I%INCLUDE_PATH% /I%THIRD_PARTY_INCLUDE_PATH% /I%SGG_HEADERS_PATH% /Fo%BUILD_PATH%\%%~nxf.obj "%%f"
)
@echo Linking Release Build (legacy)
lib %LIB_OPTIONS% %BUILD_PATH%\*.obj %LIBS% /OUT:%OUT_PATH%\%LIB_NAME%.lib

@echo Compiling Debug Build (legacy)
for /R %%f in (sgg\*.cpp) do (
    cl %COMPILER_OPTIONS% %DEBUG_MODE% /I%INCLUDE_PATH% /I%THIRD_PARTY_INCLUDE_PATH% /I%SGG_HEADERS_PATH% /Fd%OUT_PATH%\%LIB_NAME_DEBUG%.pdb /Fo%BUILD_PATH_DEBUG%\%%~nxf.obj "%%f"
)
@echo Linking Debug Build (legacy)
link /lib %LIB_OPTIONS% %BUILD_PATH_DEBUG%\*.obj %LIBS% /OUT:%OUT_PATH%\%LIB_NAME_DEBUG%.lib

@echo Compilation Finished!!
@PAUSE
