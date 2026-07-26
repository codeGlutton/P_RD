<#
.SYNOPSIS
Wave4 아트가 도착하면 이 한 줄로 열 개 배치안을 다시 굽는다.

.DESCRIPTION
임포트(75조각 덮어쓰기) -> WBP 10개 재생성 -> 1920x1080 캡처 10장.
아트 파일명과 절단 좌표가 3차와 같으므로 코드 수정 없이 덮어쓰기만 하면 된다.
캡처는 Saved/UI/CombatLayouts/ 에 남는다.
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location (Join-Path $root 'P_RD_develop_20260726')

.\Tools\RunEditorPython.ps1 -Script "$PWD\Tools\UI\import_kaykit_ui.py" -Quiet
.\Tools\RunEditorPython.ps1 -Script "$PWD\Tools\UI\build_combat_layouts.py" -Quiet
& "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "$PWD\P_RD.uproject" `
    -ExecCmds="Automation RunTests P_RD.UI.CombatLayout.Capture; Quit" `
    -unattended -nop4 -nosplash -nosound `
    -TestExit="Automation Test Queue Empty" 2>&1 | Out-Null
Write-Host "완료. 캡처: Saved\UI\CombatLayouts\"
