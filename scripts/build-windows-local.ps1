# Local Windows build script – mirrors .github/actions/build-wheel for windows-latest.
# Use this to reproduce CI failures and fix them locally.
#
# Prerequisites:
#   1. MSYS2 installed (e.g. C:\msys64). Set $env:MSYS2_ROOT if installed elsewhere.
#   2. In an MSYS2 UCRT64 shell, install deps (one-time):
#        pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkg-config \
#          mingw-w64-ucrt-x86_64-glib2 mingw-w64-ucrt-x86_64-gmp \
#          mingw-w64-ucrt-x86_64-readline mingw-w64-ucrt-x86_64-sqlite3 \
#          mingw-w64-ucrt-x86_64-libpng mingw-w64-ucrt-x86_64-gettext
#   3. Python 3.10+ on PATH (the one you want to build for / run cibuildwheel with).
#
# Usage:
#   .\scripts\build-windows-local.ps1              # build with meson only (quick check)
#   .\scripts\build-windows-local.ps1 -Wheel        # also run cibuildwheel (cp312)
#   .\scripts\build-windows-local.ps1 -Wheel -Python "cp311"   # build wheel for cp311

param(
    [switch]$Wheel,
    [string]$Python = "cp312"
)

$ErrorActionPreference = "Stop"
$msys2Root = $env:MSYS2_ROOT
if (-not $msys2Root) {
    $msys2Root = "C:\msys64"
}
$ucrt64 = Join-Path $msys2Root "ucrt64"
$ucrt64Bin = Join-Path $ucrt64 "bin"
$usrBin = Join-Path $msys2Root "usr\bin"
$pkgConfig = Join-Path $ucrt64 "lib\pkgconfig"

if (-not (Test-Path $ucrt64Bin)) {
    Write-Error "MSYS2 UCRT64 not found at $ucrt64. Install MSYS2 and set MSYS2_ROOT if needed. See script comments."
}

# UCRT64 for gcc/pkg-config; usr\bin for sh (required by credits.sh in meson.build)
$env:PATH = "$ucrt64Bin;$usrBin;$env:PATH"
$env:PKG_CONFIG_PATH = $pkgConfig
Write-Host "Using PATH (prepend): $ucrt64Bin; $usrBin"
Write-Host "PKG_CONFIG_PATH: $env:PKG_CONFIG_PATH"

# Project root = directory containing pyproject.toml
$projectRoot = $PSScriptRoot | Split-Path -Parent
Push-Location $projectRoot

# Use build dir in TEMP to avoid Meson file-lock OSError on network/WSL drives (e.g. L: or \\wsl)
$buildDir = Join-Path $env:TEMP "gnubg-build"

try {
    Write-Host "`n--- Installing Meson, Ninja, cibuildwheel ---"
    cmd /c "pip install meson ninja cibuildwheel"
    if ($LASTEXITCODE -ne 0) { throw "pip install failed (exit $LASTEXITCODE)" }

    Write-Host "`n--- Meson setup + compile (same as CI) ---"
    if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
    & meson setup $buildDir
    if ($LASTEXITCODE -ne 0) { throw "meson setup failed" }
    & meson compile -C $buildDir
    if ($LASTEXITCODE -ne 0) { throw "meson compile failed" }

    Write-Host "`n--- Clean build dir before cibuildwheel ---"
    Remove-Item -Recurse -Force $buildDir -ErrorAction SilentlyContinue

    if ($Wheel) {
        Write-Host "`n--- Building wheel(s) with cibuildwheel ---"
        $env:CIBW_SKIP = "pp* *-win32"
        $env:CIBW_BUILD = "${Python}-*"
        $env:CIBW_PLATFORM = "windows"
        $env:CIBW_TEST_REQUIRES = "pytest"
        $env:CIBW_TEST_COMMAND = "pytest {project}/tests"
        cibuildwheel --output-dir wheelhouse
        if ($LASTEXITCODE -ne 0) { throw "cibuildwheel failed" }
        Get-ChildItem wheelhouse\*.whl | ForEach-Object { Write-Host "Built: $($_.Name)" }
    }
    else {
        Write-Host "`nSkipping wheel build (use -Wheel to run cibuildwheel)."
    }
}
finally {
    Pop-Location
}

Write-Host "`nDone."
