@echo off

set CommonCompilerFlags=-MTd -nologo -Gm- -GR- -EHa- -Od -Oi -WX -wd4201 -wd4100 ^
-DFOLAYFILA_INTERNAL=1 -DFOLAYFILA_SLOW=1 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 64-bit build
REM cl %CommonCompilerFlags% ..\code\win32_folayfila.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%
REM 64-bit build 
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\code\folayfila.cpp -Fmfolayfila.map -LD /link -incremental:no -opt:ref /PDB:folayfila_%random%.pdb /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\code\win32_folayfila.cpp -Fmwin32_folayfila.map /link %CommonLinkerFlags%
popd