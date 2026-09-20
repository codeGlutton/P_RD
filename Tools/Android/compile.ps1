# UE 5.7 must already be installed/licensed on the runner; this does not install it.
[CmdletBinding()]
param(
    [string]$Project = (Join-Path $PSScriptRoot '../../P_RD.uproject'),
    [string]$Engine = $env:UE_ROOT
)
$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Engine)) { throw 'UE_ROOT must point to an installed UE 5.7 root.' }
$build = Join-Path $Engine 'Engine/Build/BatchFiles/Build.bat'
if (-not (Test-Path -LiteralPath $build -PathType Leaf)) { throw 'UE_ROOT does not contain Build.bat.' }
$projectPath = (Resolve-Path -LiteralPath $Project).Path
& $build P_RD Android Shipping "-Project=$projectPath" -architecture=arm64 -WaitMutex -NoHotReload
if ($LASTEXITCODE -ne 0) { throw "Android arm64 Shipping compilation failed ($LASTEXITCODE)." }
