param([string]$OutDir = (Join-Path $PSScriptRoot 'build'))
$ErrorActionPreference = 'Stop'
if (!$env:XEDK) { throw 'Set XEDK to your installed Xbox 360 SDK.' }
$bin = Join-Path $env:XEDK 'bin\win32'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path
& "$bin\cl.exe" /nologo /c /O2 /MT /W4 /D_XBOX /D_XBOX_VER=200 /I"$env:XEDK\include\xbox" /Fo"$OutDir\smoke.obj" "$PSScriptRoot\smoke.cpp"
if ($LASTEXITCODE) { throw 'Xbox 360 compilation failed.' }
& "$bin\link.exe" /nologo /subsystem:xbox /machine:ppcbe /out:"$OutDir\smoke.exe" /libpath:"$env:XEDK\lib\xbox" "$OutDir\smoke.obj" d3d9.lib xapilib.lib xboxkrnl.lib
if ($LASTEXITCODE) { throw 'Xbox 360 link failed.' }
& "$bin\imagexex.exe" /nologo /in:"$OutDir\smoke.exe" /out:"$OutDir\default.xex"
if ($LASTEXITCODE) { throw 'XEX generation failed.' }
Write-Output "Built $OutDir\default.xex"

