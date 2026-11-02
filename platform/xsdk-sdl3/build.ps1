param([switch]$KernelDisplay, [string]$XsdkRoot='C:\xsdk-windows', [string]$Toolchain='C:\xsdk-tools\compiler', [string]$SdlRoot='C:\SDL-xenon', [string]$Python='C:\msys64\ucrt64\bin\python.exe', [string]$OutDir=(Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference='Stop'
$displayFlags=@()
if($KernelDisplay) { $displayFlags=@('-DCOOPDX_DISPLAY_MODE=XSDK_DISPLAY_KERNEL') }
$RepoRoot=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$AudioDir=Join-Path $RepoRoot 'src\pc\audio'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path $OutDir).Path
& "$Toolchain/bin/powerpc-xsdk-elf-gcc.exe" -std=gnu17 -mcpu=cell -fno-pic -O2 -Wall -Wextra -Werror "-I$SdlRoot/include" "-I$XsdkRoot/include" "-I$AudioDir" -c "$AudioDir/audio_sdl3.c" -o "$OutDir/audio_sdl3.o"
if($LASTEXITCODE) { throw 'Audio backend compile failed' }
& "$Toolchain/bin/powerpc-xsdk-elf-g++.exe" -std=c++23 @displayFlags -mcpu=cell -fno-pic -O2 -Wall -Wextra -Werror "-I$SdlRoot/include" "-I$XsdkRoot/include" "-I$AudioDir" -static -nostdlib -T "$XsdkRoot/src/xsdk.ld" "$PSScriptRoot/main.cpp" "$OutDir/audio_sdl3.o" -o "$OutDir/coopdx-sdl3.elf" '-Wl,--start-group' "$SdlRoot/build-xsdk/libSDL3.a" -lstdc++ "$XsdkRoot/build/libxsdk.a" -lc -lm -lgcc '-Wl,--end-group'
if($LASTEXITCODE) { throw 'SDL3 harness link failed' }
& $Python "$XsdkRoot/tools/elf2xex.py" --title-name 'CoopDX SDL3 Audio Test' "$OutDir/coopdx-sdl3.elf" "$OutDir/coopdx-sdl3.xex"
if($LASTEXITCODE) { throw 'XEX packaging failed' }

