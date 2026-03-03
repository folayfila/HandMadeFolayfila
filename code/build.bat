@echo off

set CommonCompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -wd4201 -wd4100 ^
-DFOLAYFILA_INTERNAL=1 -DFOLAYFILA_SLOW=1 -FC -Z7
set CommonLinkerFlags= -opt:ref user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM 64-bit build
REM cl %CommonCompilerFlags% ..\code\win32_folayfila.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM Literally googled this just to diplay time in the .pdb file.
for /f "tokens=1-4 delims=/.- " %%a in ("%date%") do (
    set yyyy=%%d
    set mm=%%b
    set dd=%%c
)
set t=%time: =0%
set hh=%t:~0,2%
set min=%t:~3,2%
set ss=%t:~6,2%
set timestamp=%yyyy%%mm%%dd%_%hh%%min%%ss%

REM 64-bit build 
del *.pdb > NUL 2> NUL
cl %CommonCompilerFlags% ..\code\folayfila.cpp -Fmfolayfila.map -LD /link -incremental:no -opt:ref ^
/PDB:folayfila_%timestamp%.pdb /EXPORT:GameUpdateAndRender
cl %CommonCompilerFlags% ..\code\win32_folayfila.cpp -Fmwin32_folayfila.map /link %CommonLinkerFlags%
popd