@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -nologo -GR- -EHa- -Oi -W4 -wd4201 -wd4100 ^
-DFOLAYFILA_INTERNAL=1 -DFOLAYFILA_SLOW=1 -FC -Z7 -Fmwin32_folayfila.map ^
..\code\win32_folayfila.cpp /link -opt:ref user32.lib Gdi32.lib winmm.lib
popd