@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -DFOLAYFILA_INTERNAL=1 -DFOLAYFILA_SLOW=1 -FC -Zi ..\code\win32_folayfila.cpp user32.lib Gdi32.lib
popd