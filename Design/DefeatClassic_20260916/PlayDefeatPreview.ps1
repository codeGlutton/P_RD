$project = (Resolve-Path (Join-Path $PSScriptRoot '../../P_RD.uproject')).Path
$editor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor.exe'
Start-Process -FilePath $editor -ArgumentList @(
    $project, '-game', '-windowed', '-ResX=1400', '-ResY=788',
    '-ExecCmds="RD.DefeatPreview.AfterStartup"',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:StartupScripts='
)
