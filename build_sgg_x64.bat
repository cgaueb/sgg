@SETLOCAL enabledelayedexpansion
@echo off

set STUDIO_VERSION=None
set STUDIO_EDITION=None
set STUDIO_PATH=\Program Files (x86)\Microsoft Visual Studio
set DRIVE_LIST=(C D E F G H I J K L M N O P Q R S T U V W X Y Z A B)


FOR %%i IN %DRIVE_LIST% DO (
	IF NOT EXIST "!STUDIO_PATH!\2017" (
		IF NOT EXIST "!STUDIO_PATH!\2019" (
			set "STUDIO_PATH=%%i:\Program Files (x86)\Microsoft Visual Studio"	
		)
		IF EXIST "!STUDIO_PATH!\2019" (
			goto :found
		) 
	)
	IF EXIST "!STUDIO_PATH!\2017" (
		goto :found
	)
)

echo "No Visual Studio installation was found. Please make sure that Visual Studio is installed on your system."
echo "If Visual Studio is installed but in non-regular directory, please enter the path manually (STUDIO_PATH) by editing this file in a text editor."
EXIT \B

:found
IF EXIST "%STUDIO_PATH%\2017\" (
set STUDIO_VERSION=2017
)
IF EXIST "%STUDIO_PATH%\2019\" (
set STUDIO_VERSION=2019
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
    echo "Wrong Visual Studio Version Installed (2017 or 2019 required)"
    EXIT /B 1
)

IF %STUDIO_EDITION% == None (
    echo "Wrong Visual Studio Edition Installed (Community Edition is required)"
    EXIT /b 1
)

echo Compiling with Visual Studio %STUDIO_VERSION% %STUDIO_EDITION%

call "%STUDIO_PATH%\%STUDIO_VERSION%\%STUDIO_EDITION%\VC\Auxiliary\Build\vcvars64.bat"

set INCLUDE_PATH=.
set THIRD_PARTY_INCLUDE_PATH=3rdparty/include
set BUILD_PATH=build\Release
set BUILD_PATH_DEBUG=build\Debug
set OUT_PATH=lib
set LIB_NAME=sgg
set LIB_NAME_DEBUG=sggd
set THIRD_PARTY_LIB_PATH=3rdparty/lib
set COMPILER_OPTIONS=/c /EHsc /nologo
set DEBUG_MODE=/Zi /Od /DEBUG /D"_DEBUG" /MDd /std:c++17
set RELEASE_MODE=/O2 /MD /std:c++17
set LIBS=SDL2_mixer.lib glew32.lib SDL2.lib SDL2main.lib opengl32.lib freetype.lib
set LIB_OPTIONS=/LIBPATH:"%THIRD_PARTY_LIB_PATH%" /SUBSYSTEM:CONSOLE /MACHINE:X64 /ignore:4006 /nologo

echo Creating directories
IF NOT EXIST build mkdir build
IF NOT EXIST %OUT_PATH% mkdir %OUT_PATH%
IF NOT EXIST %BUILD_PATH% mkdir %BUILD_PATH%
IF NOT EXIST %BUILD_PATH_DEBUG% mkdir %BUILD_PATH_DEBUG%
echo on

@echo Compiling Release Build
cl %COMPILER_OPTIONS% %RELEASE_MODE% /I%INCLUDE_PATH% /I%THIRD_PARTY_INCLUDE_PATH% /Fo%BUILD_PATH%/ %INCLUDE_PATH%\sgg\*.cpp 
@echo Linking Relase Build
lib %LIB_OPTIONS% %BUILD_PATH%\*.obj %LIBS% /OUT:%OUT_PATH%\%LIB_NAME%.lib 

@echo Compiling Debug Build
cl %COMPILER_OPTIONS% %DEBUG_MODE% /I%INCLUDE_PATH% /I%THIRD_PARTY_INCLUDE_PATH% /Fd%OUT_PATH%\%LIB_NAME_DEBUG%.pdb /Fo%BUILD_PATH_DEBUG%/ %INCLUDE_PATH%\sgg\*.cpp
@echo Linking Debug Build
link /lib %LIB_OPTIONS% %BUILD_PATH_DEBUG%\*.obj %LIBS% /OUT:%OUT_PATH%\%LIB_NAME_DEBUG%.lib

@echo Compilation Finished!!
@PAUSE
