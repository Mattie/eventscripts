<#
.SYNOPSIS
Builds the standalone native components and runs checks when tests are present.
.DESCRIPTION
This launcher runs standalone checks, not the optional historical-ABI plugin build.
It does not start a server.
#>
param(
    [string]$CMakePath,
    [string]$PythonExecutable,
    [string]$BuildDirectory = 'build\host',
    [string]$Configuration = 'Release',
    [switch]$HostOnly
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$testsAvailable = Test-Path -LiteralPath (Join-Path $root 'tests\CMakeLists.txt') -PathType Leaf
if ($HostOnly -and -not $testsAvailable) {
    throw '-HostOnly requires the tests tree. Restore tests/CMakeLists.txt before running host checks.'
}
if (-not $CMakePath) {
    $command = Get-Command cmake.exe -ErrorAction SilentlyContinue
    if ($command) { $CMakePath = $command.Source }
    foreach ($year in @('2022', '2019')) {
        if ($CMakePath) { break }
        $candidate = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\$year\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
        if (Test-Path -LiteralPath $candidate) { $CMakePath = $candidate }
    }
}
if (-not $CMakePath -or -not (Test-Path -LiteralPath $CMakePath)) {
    throw 'CMake 3.20 or newer is required; pass its executable with -CMakePath.'
}
$testArguments = @('-DBUILD_TESTING=OFF')
if ($testsAvailable) {
    $ctest = Join-Path (Split-Path -Parent $CMakePath) 'ctest.exe'
    if (-not (Test-Path -LiteralPath $ctest)) { throw 'ctest.exe must be installed alongside CMake.' }
    if (-not $PythonExecutable) {
        $venvPython = Join-Path $root '.venv\Scripts\python.exe'
        if (Test-Path -LiteralPath $venvPython) {
            $PythonExecutable = $venvPython
        } else {
            $PythonExecutable = (Get-Command python.exe -ErrorAction Stop).Source
        }
    }
    $PythonExecutable = (Resolve-Path -LiteralPath $PythonExecutable -ErrorAction Stop).Path
    $testArguments = @('-DBUILD_TESTING=ON', "-DPython3_EXECUTABLE=$PythonExecutable")
}
if (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $BuildDirectory = Join-Path $root $BuildDirectory
}
$BuildDirectory = [IO.Path]::GetFullPath($BuildDirectory)
$outputRoot = [IO.Path]::GetFullPath((Join-Path $root 'build'))
if (-not $BuildDirectory.StartsWith($outputRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Choose a build directory beneath the project build directory.'
}

$native = if ($HostOnly) { 'OFF' } else { 'ON' }
& $CMakePath -S $root -B $BuildDirectory @testArguments '-DEVENTSCRIPTS_BUILD_PLUGIN=OFF' "-DEVENTSCRIPTS_BUILD_NATIVE=$native"
if ($LASTEXITCODE -ne 0) { throw "CMake configuration failed ($LASTEXITCODE)." }
& $CMakePath --build $BuildDirectory --config $Configuration
if ($LASTEXITCODE -ne 0) { throw "Build failed ($LASTEXITCODE)." }
if (-not $testsAvailable) {
    Write-Output 'Standalone libraries built. Restore the tests tree to run checks.'
    return
}
& $ctest --test-dir $BuildDirectory -C $Configuration --output-on-failure
if ($LASTEXITCODE -ne 0) { throw "Host checks failed ($LASTEXITCODE)." }
Write-Output 'Checks passed. Outputs are local rebuilds; no native plugin binary was built or server started.'
