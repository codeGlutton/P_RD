<#
.SYNOPSIS
Run an editor Python script and fail if it actually failed.

.DESCRIPTION
UnrealEditor-Cmd's -ExecutePythonScript exits 0 even when the script raises an
uncaught exception -- the traceback goes to the log and the process still
reports success. A Concept04 texture import silently did nothing for several
build-and-capture cycles because of exactly that.

This wrapper runs the script, then scans the log the run produced for a Python
traceback and returns a non-zero exit code if it finds one. Use it instead of
calling UnrealEditor-Cmd directly for anything whose failure would otherwise be
invisible.
#>
param(
    [Parameter(Mandatory = $true)][string]$Script,
    [string[]]$ExtraArgs = @(),
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [switch]$Quiet
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'P_RD.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$log = Join-Path $projectRoot 'Saved\Logs\P_RD.log'

if (-not (Test-Path -LiteralPath $editorCmd)) {
    throw "UnrealEditor-Cmd.exe not found: $editorCmd"
}
if (-not (Test-Path -LiteralPath $Script)) {
    throw "Python script not found: $Script"
}

$arguments = @(
    $projectFile,
    "-ExecutePythonScript=$Script",
    '-unattended', '-nop4', '-nosplash', '-nosound', '-nullrhi'
) + $ExtraArgs

& $editorCmd @arguments 2>&1 | Out-Null
$editorExit = $LASTEXITCODE

if (-not (Test-Path -LiteralPath $log)) {
    throw "Editor produced no log at $log"
}

# The traceback is the only reliable signal; the process exit code is not.
#
# Startup scripts run first and raise their own tracebacks -- SVNLinker.py
# fails in every git worktree because it writes to <project>/.git/hooks and
# .git is a file there. Failing on those would make this wrapper useless, so
# only a traceback naming the script we were asked to run counts as failure.
$errors = Select-String -Path $log -Pattern 'LogPython: Error:' -Encoding utf8
$scriptLeaf = Split-Path -Leaf $Script
$ours = $errors | Where-Object { $_.Line -like "*$scriptLeaf*" }

if ($errors -and -not $ours -and -not $Quiet) {
    Write-Host "Note: unrelated Python errors in the log (startup scripts)." -ForegroundColor DarkYellow
}
if ($ours) {
    Write-Host "Python script FAILED: $Script" -ForegroundColor Red
    # Print the whole tail of the log's error stream: the frame that names our
    # script is rarely the line that explains the failure.
    $start = [Array]::IndexOf(@($errors), @($ours)[0])
    $errors | Select-Object -Skip ([Math]::Max(0, $start - 2)) -First 30 | ForEach-Object {
        Write-Host ("  " + (($_.Line -split 'LogPython: Error:\s*')[-1]))
    }
    exit 1
}
if ($editorExit -ne 0) {
    Write-Host "Editor exited $editorExit for $Script" -ForegroundColor Red
    exit $editorExit
}
if (-not $Quiet) {
    Write-Host "OK: $Script"
}
exit 0
