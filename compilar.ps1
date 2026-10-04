$ErrorActionPreference = 'Stop'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $PSScriptRoot 'tools/cache'
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $PSScriptRoot 'tools/local-cache'
$compiler = Join-Path $PSScriptRoot 'tools/zig-windows-x86_64-0.13.0/zig.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw 'Baixe e extraia Zig 0.13.0 em tools antes de compilar.' }
New-Item -ItemType Directory -Force (Join-Path $PSScriptRoot 'dist') | Out-Null
& $compiler c++ -std=c++17 -O2 -s -target x86_64-windows-gnu (Join-Path $PSScriptRoot 'src/main.cpp') -o (Join-Path $PSScriptRoot 'dist/DoceHamburgueria-Windows.exe')
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar Windows.' }
& $compiler c++ -std=c++17 -O2 -s -target x86_64-linux-musl -static (Join-Path $PSScriptRoot 'src/main.cpp') -o (Join-Path $PSScriptRoot 'dist/DoceHamburgueria-Linux')
if ($LASTEXITCODE -ne 0) { throw 'Falha ao compilar Linux.' }
$importLibrary = Join-Path $PSScriptRoot 'dist/main.lib'
if (Test-Path -LiteralPath $importLibrary) { Remove-Item -LiteralPath $importLibrary }
