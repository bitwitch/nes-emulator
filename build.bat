@echo off
if not exist build\ mkdir build
pushd build

setlocal EnableExtensions EnableDelayedExpansion
set URL=https://github.com/libsdl-org/SDL/releases/download/release-2.26.4/SDL2-devel-2.26.4-VC.zip
set ZIPFILE="%CD%\SDL2-devel-2.26.4-VC.zip"

REM copy SDL2 libraries to build directory if they haven't already been
if not exist SDL2\ (
	if exist ..\SDL2\ (
		xcopy /D /E /I ..\SDL2\include SDL2\include\SDL2
		echo F | xcopy /D ..\SDL2\lib\x64\SDL2.lib SDL2\lib\SDL2.lib
		echo F | xcopy /D ..\SDL2\lib\x64\SDL2.dll .

REM download SDL2 development libraries
	) else (
		bitsadmin /transfer downloadSDL2 /download %URL% %ZIPFILE%
		if not exist %ZIPFILE% (
			echo Build Error: Failed to download SDL2. 
			echo You can manually download SDL2 development libraries for VC, extract the contents, and move them to a directory called SDL2 in the project root.
			echo https://github.com/libsdl-org/SDL/releases/
			endlocal
			popd
			exit /b 1
		)

REM unzip downloaded SDL2 zip file
		powershell -nologo -noprofile -command "Expand-Archive -Path '%ZIPFILE%' -DestinationPath '%CD%\..'"
		if not exist ..\SDL2-2.26.4 (	
			echo "Failed to unzip %ZIPFILE%"
			echo "You can manually extract the contents, and move them to a directory called SDL2 in the project root."
			endlocal
			popd
			exit /b 1
		)
		move ..\SDL2-2.26.4 ..\SDL2
		del %ZIPFILE%

REM copy SDL2 libraries to build directory
		xcopy /D /E /I ..\SDL2\include SDL2\include\SDL2
		echo F | xcopy /D ..\SDL2\lib\x64\SDL2.lib SDL2\lib\SDL2.lib
		echo F | xcopy /D ..\SDL2\lib\x64\SDL2.dll .
	)
)

if not exist inconsolata.ttf xcopy /D ..\inconsolata.ttf .

cl "%~dp0src\main.c" /DSDL_MAIN_HANDLED SDL2.lib /D_CRT_SECURE_NO_WARNINGS /Zi /W3 /nologo /Fenes /I.. /I..\src /ISDL2\include /link /LIBPATH:SDL2\lib

endlocal
popd
