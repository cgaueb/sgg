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
