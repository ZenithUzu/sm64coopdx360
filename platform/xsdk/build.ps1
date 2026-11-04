param(
    [switch]$Game,
    [string]$RomPath='',
    [string]$XsdkRoot='C:\xsdk-windows',
    [string]$Toolchain='C:\xsdk-tools\compiler',
    [string]$SdlBuild='C:\SDL-xenon\build-xsdk',
    [string]$MsysRoot='C:\msys64',
    [string]$Glslang='',
    [ValidateRange(1,64)][int]$Jobs=4,
    [ValidateSet('Release','RelWithDebInfo','Debug','MinSizeRel')][string]$BuildType='Release',
    [switch]$RestoreInheritedInputs,
    [string]$OutDir=(Join-Path $PSScriptRoot 'build')
)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$cmake=Join-Path $MsysRoot 'ucrt64/bin/cmake.exe'
$python=Join-Path $MsysRoot 'ucrt64/bin/python.exe'
$ninja=Join-Path $MsysRoot 'ucrt64/bin/ninja.exe'
$make=Join-Path $MsysRoot 'usr/bin/make.exe'
if(!$Glslang) { $Glslang=Join-Path $Toolchain 'bin/glslang.exe' }
$required=@($cmake,$python,$ninja,$Glslang,
    "$Toolchain/bin/powerpc-xsdk-elf-gcc.exe", "$Toolchain/bin/powerpc-xsdk-elf-g++.exe",
    "$XsdkRoot/cmake/xsdk.cmake", "$XsdkRoot/build/libxsdk.a", "$XsdkRoot/build/host/libxsc.dll")
if($Game) { $required+=$make }
foreach($file in $required) {
    if(!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing prerequisite: $file. Install/build that component first." }
}
if(!(Test-Path -LiteralPath $SdlBuild -PathType Container)) { throw 'Select the built SDL3 Xenon package with -SdlBuild.' }
$XsdkRoot=(Resolve-Path -LiteralPath $XsdkRoot).Path
$Toolchain=(Resolve-Path -LiteralPath $Toolchain).Path
$Glslang=(Resolve-Path -LiteralPath $Glslang).Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$previousPath=$env:PATH
Push-Location $repo
try {
    # Both GCC and the asset utilities need the UCRT64 DLL search path.
    $env:PATH="$MsysRoot\ucrt64\bin;$MsysRoot\usr\bin;$Toolchain\bin;"+$previousPath
    if($Game) {
        if($RomPath) {
            $expected=(Get-Content (Join-Path $repo 'sm64.us.sha1')).Split(' ')[0].Trim()
            if((Get-FileHash -LiteralPath $RomPath -Algorithm SHA1).Hash.ToLowerInvariant() -ne $expected) {
                throw 'ROM must match the US cartridge dump expected by sm64.us.sha1.'
            }
            $destination=Join-Path $repo 'baserom.us.z64'
            if((Resolve-Path -LiteralPath $RomPath).Path -ne $destination) {
                Copy-Item -LiteralPath $RomPath -Destination $destination
            }
        }
        if(!(Test-Path -LiteralPath (Join-Path $repo 'baserom.us.z64'))) {
            throw 'Supply your own US ROM with -RomPath. It remains an ignored local build input.'
        }
        if($RestoreInheritedInputs) {
            & $python "$PSScriptRoot/restore_inherited_inputs.py"
            if($LASTEXITCODE) { throw 'Local inherited inputs could not be restored; use a full clone and check permissions.' }
        }
        & $make -C tools "-j$Jobs" n64graphics skyconv mio0 aifc_decode textconv aiff_extract_codebook vadpcm_enc
        if($LASTEXITCODE) { throw 'Host asset tools failed' }
        & $python extract_assets.py us
        if($LASTEXITCODE) { throw 'ROM asset extraction failed' }
        & $python "$PSScriptRoot/prepare_assets.py" --repo $repo --out "$OutDir/generated" --compiler "$Toolchain/bin/powerpc-xsdk-elf-gcc.exe"
        if($LASTEXITCODE) { throw 'Generated game assets failed. Supply the private inputs described in docs/xbox360/BUILD.md.' }
    }
    $gameOption=if($Game) { 'ON' } else { 'OFF' }
    & $cmake -S $PSScriptRoot -B $OutDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" `
        "-DCMAKE_TOOLCHAIN_FILE=$XsdkRoot/cmake/xsdk.cmake" "-DXSDK_TOOLCHAIN=$Toolchain" `
        "-DSDL3_DIR=$SdlBuild" "-DCOOPDX_BUILD_GAME=$gameOption" "-DCMAKE_BUILD_TYPE=$BuildType" `
        "-DCOOPDX_PYTHON=$python" "-DCOOPDX_GLSLANG=$Glslang"
    if($LASTEXITCODE) { throw 'xSDK CMake configure failed' }
    if($Game) { & $cmake --build $OutDir --target sm64coopdx --parallel $Jobs }
    else { & $cmake --build $OutDir --parallel $Jobs }
    if($LASTEXITCODE) { throw 'xSDK build failed' }
    if($Game) {
        $files=@{}
        foreach($name in @('sm64coopdx.xex','sm64coopdx','sm64coopdx.elf')) {
            $file=Join-Path $OutDir $name
            if(Test-Path -LiteralPath $file) { $files[$name]=(Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash.ToLowerInvariant() }
        }
        $commit=(& git rev-parse HEAD 2>$null | Out-String).Trim()
        $dirty=(@(& git status --porcelain --untracked-files=no 2>$null).Count -gt 0)
        $report=[ordered]@{
            schema=1; profile='coopdx-xsdk-source'; build_type=$BuildType; source_commit=$commit; source_dirty=$dirty
            built_utc=[DateTime]::UtcNow.ToString('o'); hardware_validation='not-run'; sha256=$files
        }
        $report | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 (Join-Path $OutDir 'build-info.json')
        Write-Output "Game image: $OutDir\sm64coopdx.xex"
        Write-Output 'Compilation complete; console performance and connectivity still require hardware testing.'
    }
} finally {
    $env:PATH=$previousPath
    Pop-Location
}
