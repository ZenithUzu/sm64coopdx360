param(
    [switch]$Game,
    [string]$RomPath='',
    [string]$XsdkRoot='C:\xsdk-windows',
    [string]$Toolchain='C:\xsdk-tools\compiler',
    [string]$SdlBuild='C:\SDL-xenon\build-xsdk',
    [string]$OutDir=(Join-Path $PSScriptRoot 'build')
)
$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$cmake='C:\msys64\ucrt64\bin\cmake.exe'
$python='C:\msys64\ucrt64\bin\python.exe'
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir=(Resolve-Path $OutDir).Path
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
    $previousPath=$env:PATH
    Push-Location $repo
    try {
        $env:PATH='C:\msys64\ucrt64\bin;C:\msys64\usr\bin;'+$previousPath
        & C:\msys64\usr\bin\make.exe -C tools -j4 n64graphics skyconv mio0 aifc_decode textconv aiff_extract_codebook vadpcm_enc
        if($LASTEXITCODE) { throw 'Host asset tools failed' }
        & $python extract_assets.py us
        if($LASTEXITCODE) { throw 'ROM asset extraction failed' }
        & $python "$PSScriptRoot/prepare_assets.py" --repo $repo --out "$OutDir/generated" --compiler "$Toolchain/bin/powerpc-xsdk-elf-gcc.exe"
        if($LASTEXITCODE) { throw 'Generated game assets failed' }
    } finally {
        $env:PATH=$previousPath
        Pop-Location
    }
}
# The PowerPC compiler loads its DLLs from UCRT64.
$env:PATH='C:\msys64\ucrt64\bin;'+$env:PATH
$gameOption=if($Game) { 'ON' } else { 'OFF' }
& $cmake -S $PSScriptRoot -B $OutDir -G Ninja '-DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/ninja.exe' "-DCMAKE_TOOLCHAIN_FILE=$XsdkRoot/cmake/xsdk.cmake" "-DXSDK_TOOLCHAIN=$Toolchain" "-DSDL3_DIR=$SdlBuild" "-DCOOPDX_BUILD_GAME=$gameOption" -DCMAKE_BUILD_TYPE=Release
if($LASTEXITCODE) { throw 'xSDK CMake configure failed' }
if($Game) {
    & $cmake --build $OutDir --target sm64coopdx --parallel 6
} else {
    & $cmake --build $OutDir --parallel 6
}
if($LASTEXITCODE) { throw 'xSDK build failed' }
if($Game) { Write-Output "Game image: $OutDir\sm64coopdx.xex" }
