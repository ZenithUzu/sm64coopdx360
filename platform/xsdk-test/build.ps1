param([string]$XsdkRoot='C:\xsdk-windows', [string]$Toolchain='C:\xsdk-tools\compiler', [string]$Python='C:\msys64\ucrt64\bin\python.exe', [string]$OutDir=(Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path $OutDir).Path
& "$Toolchain\bin\powerpc-xsdk-elf-g++.exe" -std=c++23 -mcpu=cell -fno-pic -O2 -Wall -Wextra -Werror "-I$XsdkRoot/include" -static -nostdlib -T "$XsdkRoot/src/xsdk.ld" "$PSScriptRoot/main.cpp" -o "$OutDir/xsdk-test.elf" '-Wl,--start-group' -lstdc++ "$XsdkRoot/build/libxsdk.a" -lc -lm -lgcc '-Wl,--end-group'
if ($LASTEXITCODE) { throw 'xSDK compilation/link failed' }
& $Python "$XsdkRoot/tools/elf2xex.py" --title-name 'SM64CoopDX xSDK Test' "$OutDir/xsdk-test.elf" "$OutDir/xsdk-test.xex"
if ($LASTEXITCODE) { throw 'XEX packaging failed' }
