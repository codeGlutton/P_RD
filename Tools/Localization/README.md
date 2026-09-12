# 스킬 번역 갱신 및 검증

스킬 이름·설명 또는 설명을 생성하는 `LOCTEXT` 원문이 바뀌면 번역 리소스를 함께 갱신합니다. 기존 키에 번역이 남아 있더라도 **원문이 달라지면 런타임에서 번역이 적용되지 않을 수 있습니다.**

## 갱신 순서

1. 사용할 Git/SVN 버전을 확인하고 프로젝트의 에디터 모듈을 빌드합니다.
2. 아래 GatherText 명령으로 현재 코드·에셋의 문자열을 수집합니다.
3. `Content/Localization/Game/en/Game.po`와 `ko/Game.po`의 신규·변경 항목을 번역합니다. 이 프로젝트는 `NativeCulture=en`이지만 한국어 원문도 사용하므로 en 파일의 영문 번역도 필요합니다.
4. 같은 GatherText 명령을 다시 실행하여 PO를 가져오고 런타임 `locres`를 생성합니다.
5. archive 검사와 실제 스킬 언어 전환 테스트를 실행합니다. manifest, 한·영 archive/PO/locres를 함께 커밋합니다.

```powershell
$editor = 'C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$project = (Resolve-Path '.\P_RD.uproject').Path
$report = Join-Path (Split-Path $project) 'Saved\Automation\SkillLocalization'

svnversion .\Content\SVN
& $editor $project -run=GatherText '-config=Config/Localization/Game.ini' `
    -Unattended -NullRHI -NoSound
if ($LASTEXITCODE -ne 0) { throw 'GatherText failed' }
python Tools/Localization/audit_translations.py
if ($LASTEXITCODE -ne 0) { throw 'Translation audit failed' }

& $editor $project /Engine/Maps/Entry `
    -Unattended -NullRHI -NoSound `
    '-ExecCmds=Automation RunTests P_RD.UI.Localization.SkillsAndReplacement+P_RD.Assets.GolemSkillReferences' `
    '-TestExit=Automation Test Queue Empty' `
    "-ReportExportPath=$report"
svnversion .\Content\SVN
```

자동화 결과의 `tests[].state`와 오류를 확인합니다. Unreal 프로세스 종료 코드가 0이어도 자동화 테스트는 실패할 수 있습니다. 언어 전환 테스트는 영어 → 한국어 → 영어 순서로 실제 스킬의 이름, 저장된 `mDescription`, `MakeDescription()` 결과를 검사합니다.

격리 worktree에서는 원하는 `Content/SVN` 연결을 먼저 준비해야 합니다. 프로젝트 시작 스크립트 `SVNLinker.py`가 그 연결을 기본 폴더로 바꿀 수 있으므로 실행 전후의 SVN 버전을 비교합니다. 검증용 복사본에서 시작 스크립트를 잠시 제외하려면 `Config/DefaultEngine.ini` 끝에 다음 설정을 추가하고, 검증 후 원래 파일을 복원합니다. 이 임시 설정은 커밋하지 않습니다.

```ini
[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
!StartupScripts=ClearArray
```

## 에셋 이름 변경

파일 탐색기에서 `.uasset` 이름만 바꾸지 말고 Unreal Editor의 이름 변경 기능을 사용합니다. 저장된 참조를 갱신하고 이전 redirector의 참조가 남지 않았는지 확인한 다음 번역을 재수집합니다.

이미 배포한 스킬의 에셋 이름을 바꾼 경우에는 저장된 `FPrimaryAssetId`도 고려해야 합니다. 골렘의 `DA_Goelm_*` → `DA_Golem_*` 변경은 `DefaultGame.ini`의 `PrimaryAssetIdRedirects`로 이전 ID를 유지하며, `GolemSkillReferences` 테스트가 실제 에셋 및 유닛 참조와 함께 검사합니다.
