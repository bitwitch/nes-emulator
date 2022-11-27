@echo off
mkdir .\build-windows
pushd .\build-windows

xcopy /D ..\SDL2\bin\SDL2.dll .\SDL2.dll
xcopy /E /I /D ..\assets .\assets

setlocal EnableDelayedExpansion
setlocal EnableExtensions
set sources=
for %%i in (..\src\*) do set sources=!sources! %%i

gcc -Wall -g %sources% -I..\SDL2\include -I.. -I..\src -L..\SDL2\lib -lmingw32 -lSDL2main -lSDL2 -o nes

popd
