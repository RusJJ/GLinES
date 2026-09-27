param([string]$NDKPath = $env:ANDROID_NDK_HOME, [int]$Jobs = 4)
$ErrorActionPreference = "Stop"

if (-not $NDKPath) { $NDKPath = $env:ANDROID_NDK_ROOT }
if (-not $NDKPath -and (Test-Path "$PSScriptRoot/NDKPath.txt")) {
    $NDKPath = (Get-Content "$PSScriptRoot/NDKPath.txt" -Raw).Trim()
}
if (-not $NDKPath) { throw "Pass -NDKPath or set ANDROID_NDK_HOME." }
$command = if ($env:OS -eq "Windows_NT") { "ndk-build.cmd" } else { "ndk-build" }
$buildScript = Join-Path $NDKPath $command
if (-not (Test-Path $buildScript)) { throw "ndk-build was not found at $buildScript" }

& $buildScript "NDK_PROJECT_PATH=$PSScriptRoot" "APP_BUILD_SCRIPT=$PSScriptRoot/Android.mk" "NDK_APPLICATION_MK=$PSScriptRoot/Application.mk" NDK_DEBUG=0 "-j$Jobs"
$buildExit = $LASTEXITCODE
if ($buildExit -ne 0) { throw "NDK build failed with exit code $buildExit" }
Write-Output "Build complete. Libraries are in $PSScriptRoot/libs."
