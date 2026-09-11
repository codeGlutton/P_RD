# 크레딧·라이선스 화면

타이틀 및 인게임 설정의 왼쪽 페이지 아래에 **크레딧**, **라이선스** 버튼을 제공합니다. 기존 SettingsLedger 책·나무 버튼·파란 리본, KitA 금속 테두리 버튼, Hire 구분선과 LINE Seed KR 글꼴을 사용합니다. 제작자 이름, 작품명, 출처 링크의 크기와 굵기를 나누어 명단을 읽기 쉽게 표시합니다.

- 크레딧: 모델·애니메이션, 이미지·AI 제작, 음악·효과음, 시각 효과, 글꼴, 엔진의 6개 분류. 출처 주소는 누르면 브라우저에서 열립니다.
- 이미지·AI 제작: 사용자 확인에 따라 UI 그래픽과 아이콘, 그 밖의 여러 이미지 제작에 생성형 AI를 사용했다는 사실을 한국어/영어로 표시합니다. `AI_KO.txt`와 `AI.txt`가 각각 해당 언어의 문서입니다. 이미지 생성 서비스의 이름은 확인되지 않아 임의로 표기하지 않았습니다.
- 모델·애니메이션에는 사용자가 전달한 함정 메시 제작자 확인에 따라 `Some 3D assets were created using Tencent Hunyuan 3D.`를 포함합니다. 한국어/영어 화면 모두 같은 원문을 표시합니다.
- 라이선스: 고운바탕, LINE Seed KR, Oswald, Roboto, Noto Arabic/Thai, Droid Sans, Last Resort의 저작권 고지와 약관 전문.
- 좌측 분류와 우측 문서는 각각 스크롤됩니다. 한국어/영어 UI를 지원하며 라이선스 전문은 원문으로 표시합니다.
- 뒤로가기 버튼, Escape, Android Back, 게임패드 취소로 설정에 돌아옵니다. 읽는 동안 아래 설정 입력을 차단하고, 설정이 닫히면 읽기 화면도 닫습니다.

## 배포 데이터

`Content/Legal/Credits/*.txt`와 `Content/Legal/Licenses/*.txt`를 직접 편집할 수 있습니다. `P_RD.Build.cs`에서 이 파일을 UFS 런타임 의존성에 추가하므로, FileHelper로 오프라인에서 읽습니다. 신규 문서를 추가할 때는 화면의 문서 목록도 갱신해야 합니다.

기준 자료는 `D:/Builds/P_RD/AssetLicenses_20260910/`의 2026-09-10 APK 감사 결과입니다. 제작자가 식별된 사용 팩을 감사 표기에 포함했으며, 그 표기가 취득 기록 확인을 의미하지는 않습니다. 취득 기록 미확인 목록, 출처 미확정 아이콘/AI 미디어와 오디오, 최종 AAB 대조 및 엔진·플러그인 전체의 제3자 고지 검토는 기존 감사의 남은 항목입니다. 플레이어용 문서에는 내부 조사 메모를 넣지 않았습니다.

`CreditsContentManifest.json`은 폰트 약관 원본과 배포 문서의 SHA-256을 기록합니다. Roboto 및 Last Resort 원문은 Windows-1252에서 UTF-8로 손실 없이 변환했고 엔진 폰트 저작권 고지는 원문 앞에 추가했습니다. OFL 본문은 변경하지 않았습니다.

## 검증

- 빌드: `P_RDEditor Win64 Development`
- 자동화: `P_RD.UI.Credits` (실제 설정 버튼 진입/복귀, 두 설정 모드와 언어, 문서 로딩, 화면 캡처)
- 회귀 확인: `P_RD.UI.Settings.BackLifecycle`, `P_RD.UI.Settings.Capture`
- 화면 캡처: `Saved/UI/Credits/`, 가로 1672×941 및 폴드 2176×1812, 한국어/영어와 두 탭. 크레딧 및 설정 캡처는 실행별 시각·GUID 하위 폴더에 보존합니다.

에디터 테스트 실행 시 `-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:-StartupScripts=SVNLinker.py`를 전달하여 해당 프로세스의 시작 스크립트 목록에서 SVNLinker를 제외합니다. 프로젝트 설정 파일은 변경하지 않습니다. Android AAB 설치 및 기기에서의 터치·하드웨어 Back 검증은 별도로 수행해야 합니다.

2026-09-12 검증 결과: 설정 회귀를 포함한 위 자동화 5개가 통과했고, 이미지·AI 제작 추가 후 `P_RD.UI.Credits` 3개를 다시 실행해 모두 통과했습니다(실패 0). 에디터 월드에서 뷰포트를 여는 테스트 경로의 World 경고가 있으며 실제 Android 기기 실행 결과는 아닙니다. 빌드 receipt에 AI 사용 고지의 한국어/영어 문서를 포함한 Legal 문서 14개가 UFS로 포함된 것을 확인했고, 폰트 약관 7개의 해시를 대조했습니다. AI 사용 고지 화면도 두 언어와 두 화면 비율로 캡처했습니다.
