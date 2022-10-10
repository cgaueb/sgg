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

set INCLUDE_PATH=/I. /I3rdparty/include
set BUILD_PATH=build\Release
set BUILD_PATH_DEBUG=build\Debug
set OUT_PATH=bin
set LIB_NAME=sgg.lib
set LIB_NAME_DEBUG=sggd.lib
set EXE_NAME=demo.exe
set EXE_NAME_DEBUG=demod.exe
set LIB_PATH=lib
set SRC_PATH=demo
set COMPILER_OPTIONS=/EHsc /nologo
set DEBUG_MODE=/Zi /DEBUG /D"_DEBUG" /MDd /std:c++17
set RELEASE_MODE=/O2 /MD /std:c++17
set LIB_OPTIONS=/LIBPATH:"%LIB_PATH%" /SUBSYSTEM:CONSOLE /MACHINE:X64 /nologo

echo Creating directories
IF NOT EXIST build mkdir build
IF NOT EXIST %OUT_PATH% mkdir %OUT_PATH%
IF NOT EXIST %OUT_PATH%\assets mkdir %OUT_PATH%\assets
IF NOT EXIST %BUILD_PATH% mkdir %BUILD_PATH%
IF NOT EXIST %BUILD_PATH_DEBUG% mkdir %BUILD_PATH_DEBUG%
echo on

cl %COMPILER_OPTIONS% %RELEASE_MODE% %INCLUDE_PATH% /Fo%BUILD_PATH%/  %SRC_PATH%\*.cpp /link %LIB_OPTIONS% %LIB_NAME%  /OUT:"%OUT_PATH%\%EXE_NAME%"
cl %COMPILER_OPTIONS% %DEBUG_MODE% %INCLUDE_PATH% /Fd%OUT_PATH%\%LIB_NAME_DEBUG%.pdb /Fo%BUILD_PATH_DEBUG%/  %SRC_PATH%\*.cpp /link %LIB_OPTIONS% %LIB_NAME_DEBUG% /OUT:"%OUT_PATH%\%EXE_NAME_DEBUG%"

@echo Copying Binaries
@XCOPY /s /Y 3rdparty\bin %OUT_PATH% > nul
@echo Copying Assets
@XCOPY /s /Y 3rdparty\assets %OUT_PATH%\assets > nul

@echo Compilation Finished!!
@PAUSE
