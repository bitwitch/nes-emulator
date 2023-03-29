@echo off
pushd .\build-windows
xcopy /D ..\font.png .
setlocal EnableExtensions EnableDelayedExpansion
set sources=
for %%i in (..\src\*.c) do set sources=!sources! %%i

cl %sources% /DSDL_MAIN_HANDLED SDL2main.lib SDL2.lib /Zi /W2 /nologo /Fenes /I.. /I..\src /ISDL2\include /link /LIBPATH:SDL2\lib

endlocal
popd