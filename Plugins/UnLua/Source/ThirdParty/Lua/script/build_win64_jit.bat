@echo off
if /i "%1"==debug (
    set build_config=Debug
) else (
    set build_config=Release
)
set output_dir=../lib-c/Win64/%build_config%
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"
cd ../LuaJIT-2.1.0-beta3/src

if %build_config%==Release (
    call msvcbuild.bat gc64
) else (
    call msvcbuild.bat gc64 debug
)

if not exist %output_dir% (
    mkdir "%output_dir%"
)
copy /y Lua.dll "%output_dir%/Lua.dll"
copy /y Lua.pdb "%output_dir%/Lua.pdb"
copy /y Lua.lib "%output_dir%/Lua.lib"
@pause