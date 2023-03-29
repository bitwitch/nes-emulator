@echo off
pushd .\build-windows
if not exist font.png xcopy /D ..\font.png .

REM prompt user to download SDL2 if not found
if not exist ..\SDL2\ (
	echo Build Error: Could not find SDL2 folder. Download SDL2 development libraries for VC, extract the contents, and move them to a directory called SDL2 in the project root.
	echo https://github.com/libsdl-org/SDL/releases/
	popd
	exit /b 1
)
REM copy SDL2 libraries to build directory if they haven't already been
if not exist SDL2\ (
	xcopy /D /E /I ..\SDL2\include SDL2\include\SDL2
	echo F | xcopy /D ..\SDL2\lib\x64\SDL2.lib SDL2\lib\SDL2.lib
	echo F | xcopy /D ..\SDL2\lib\x64\SDL2main.lib SDL2\lib\SDL2main.lib
	echo F | xcopy /D ..\SDL2\lib\x64\SDL2.dll .
)

setlocal EnableExtensions EnableDelayedExpansion
set sources=
for %%i in (..\src\*.c) do set sources=!sources! %%i

cl %sources% /DSDL_MAIN_HANDLED SDL2main.lib SDL2.lib /Zi /W2 /nologo /Fenes /I.. /I..\src /ISDL2\include /link /LIBPATH:SDL2\lib

endlocal
popd