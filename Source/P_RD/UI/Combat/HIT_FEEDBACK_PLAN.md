# 타격 피드백 구현 계획

> 2026-07-30 작성. "맞는 순간이 화면에 안 남는다"를 고치기 위한 구체 스펙.
> 원칙: **신규 제작 최소화** — 이미 프로젝트에 있는 위젯·아이콘·폰트·사운드를 배선하고, 새로 만드는 에셋은 카메라 셰이크 BP 2개뿐이다.

---

## 1. 실행 데미지 숫자 (최우선 — 코드 배선만으로 해결)

**현황**: 플로팅 로그 위젯·순차 재생 큐·아이콘 변환까지 전부 완성돼 있다
(`CombatLayoutHUDWidget_CombatLog.cpp`). 그런데 발행이 **미리보기 경로뿐**이다:

- 미리보기(조준 시): `OnPostSimulateSkillAction` → `PushSimulationFloatingLogs(Logs)` — `CombatGameMode.cpp:457-458` ✅
- 실행(실제 타격): `PushSimulationFloatingLogs(Logs, /*IsPreview=*/false)` — **호출부 없음** ❌

**배선 지점 (권장)**: 라이브 커밋 지점인 `USkillComponentModel::TriggerMotionLayer`
(AnimNotify_EventTrigger가 모션마다 트리거)에서 모션 단위 이벤트 로그를 델리게이트로 올린다.
미리보기의 `OnPostSimulateSkillAction`과 대칭인 `OnPostCommitMotion(EventLogs)` 델리게이트를
스킬 컴포넌트에 추가하고, `ACombatGameMode`가 파티/적 전 유닛에 구독(기존 스킬 델리게이트 연결부
`CombatGameMode.cpp:331-` 근처)해 `PushSimulationFloatingLogs(Logs, false)`를 호출.

- 차선책: `OnEndAnyTurnActionUI`(`CombatGameMode.cpp:319`)에서 액션 로그를 일괄 발행.
  구현은 더 쉽지만 다단 모션 스킬에서 숫자가 "액션 끝난 뒤 몰아서" 뜬다. 모션 단위가 정답.
- 실행 로그 큐의 간격·상승·페이드 연출은 이미 구현돼 있으므로 파라미터 튜닝만.

**표기 스타일** (전부 기존 에셋):

| 항목 | 값 |
|---|---|
| 숫자 폰트 | `F_HUD_Oswald` (`SVN/OutSideAsset/Fonts/F_HUD_Oswald.uasset`) — 숫자 전용, HUD 폰트 패밀리로 이미 준비됨 |
| 텍스트 라벨(상태이상 등) | `F_HUD_LINESeedKR` (LINESeedKR-Bold) |
| 아이콘 | `T_Status_*` 8종 — 이미 HUD 생성자에서 로드 중 (`CombatLayoutHUDWidget.cpp:41-48`), 신규 불필요 |
| 색 | 피해 빨강 / 회복 초록 / 방어 파랑 / 이동력 노랑 / 태그(버프·디버프) 보라 — `EFloatingLogColorType` 변환부에 이미 있음 |
| 크리티컬 | 폰트 크기 1.3배 + 스케일 펀치(0.1s). 크리 판정 플래그가 로그에 실리기 전까지는 보류 |

## 2. 카메라 셰이크 (신규 에셋 = BP 2개)

**현황**: 직교 전용 셰이크 패턴 `UOrthographicCameraShakePattern`(OrthoWidth까지 흔듦)과
`UCameraMovementComponent::StartCameraShake(TSubclassOf<UCameraShakeBase>)`
(`CameraMovementComponent.cpp:643`) 완성. 설정 온오프 토글도 이미 있음(`RDGameModeBase.cpp:169-173`).
**호출부만 없다.**

**만들 에셋** (`Content/BP/Camera/` 에 BP_CombatCamera 옆):

| 에셋 | 용도 | 파라미터 제안 |
|---|---|---|
| `CS_HitLight` | 일반 타격 (HP 감소 발생 시) | 진폭 소, 지속 0.15s, OrthoWidth 진폭 ~20 |
| `CS_HitHeavy` | 큰 타격(피해 ≥ 대상 MaxHP 30%) / 처치 | 진폭 대, 지속 0.25s, OrthoWidth 진폭 ~50 |

**배선 지점**: 1번과 같은 커밋 지점. HP 감소 로그(`FSRPGAttributeEffectEventLog`)를 감지하면
`CombatCameraPawn`의 `UCameraMovementComponent`에 `StartCameraShake(CS_HitLight/Heavy)`.
힐·버프·이동에는 걸지 않는다(Attack 이펙트 한정). 시뮬레이션(미리보기) 경로에서는 호출 금지.

## 3. 히트 VFX / SFX (에셋 이미 있음 — 애님 배치 확인)

자체 제작분이 이미 있다 (`Content/SVN/InSideAsset/`):

| 종류 | 에셋 | 매핑 |
|---|---|---|
| VFX | `VFX/Hit/FXS_Hit_Attack`, `FXS_Hit_Spell` | `ESkillType` Attack / Spell |
| SFX | `SFX/Hit/SFX_Hit_Attack`, `SFX_Hit_Spell` | 동일 |
| 스킬 SFX | `SFX/Skill/SFX_Skill_{Buff,DeBuff,Defense,Heal,Movement,Sword_01,Sword_02}` | 스킬 이펙트 레이어별 |

`AnimNotify_ConditionalPlayNiagaraEffect` / `AnimNotify_EventTrigger`가 이미 있으므로
**스킬 애님 24종(직업 5 × 스킬 + 몬스터)에 히트 노티파이가 실제로 박혀 있는지 에디터에서 전수 확인**이
작업의 본체다. 빠진 애님에 노티파이 배치(에디터 작업, 코드 무변경).

## 4. WBP_CombatHUD04 위젯 이름 정정 (에디터 작업, 코드 무변경)

C++는 이름으로 찾고 없으면 조용히 건너뛴다. 아래는 **코드는 완성인데 WBP에 이름이 없거나 달라서
화면에 안 나오는** 기능들. UMG 에디터에서 이름만 맞추면 켜진다.

| 코드가 찾는 이름 | WBP04 현재 | 조치 | 우선순위 |
|---|---|---|---|
| `CommandCooldownIcon_0..5` | `CommandCooldownBadge_0..5` | **이름 변경** (이동 카드에 모래시계 잔존 버그 해결) | ★★★ |
| `EnemyStatus` | 없음 | 적 안내판에 TextBlock 추가 (적 상태이상 텍스트) | ★★★ |
| `ObjectiveText` | 없음 (`ObjectivePanel`만 존재) | 패널 안에 TextBlock 추가 ("남은 적 N" 표시) | ★★ |
| `PartyStatus_0..2` | 없음 | 파티 칸에 상태이상 텍스트 추가 ("약화 2턴") | ★★ |
| `PartyAPPip_{slot}_{n}` | 없음 (숫자판만) | 낱개 AP pip — 숫자로 충분하면 생략 가능 | ★ |
| `TurnName_0..5` | 없음 | 턴 토큰 이름 — 초상화로 충분하면 생략 가능 | ★ |
| `CommandCostLine_0..5` | 없음 | 카드 비용 문장 — 숫자로 충분하면 생략 가능 | ★ |

폰트는 기존 HUD와 동일하게 `F_HUD_LINESeedKR`(한글) / `F_HUD_Oswald`(숫자).

## 5. 후순위 (이번 범위 밖, 기록만)

- **사망 연출**: 사망 시 머리 위 HP바 페이드아웃 + `CS_HitHeavy` + 처치 SFX. 애님 레이어의
  `mDefaultDeathAnim`은 이미 재생되므로 UI/카메라 쪽만.
- **크리티컬 연출**: 피해 계산에 크리 분기가 생긴 뒤 (현재 UI가 ×1.5를 임시 곱셈 중 — `CombatGameMode.cpp` 주석 참고).

## 진행 순서 제안

1. §4 이름 정정 2건(★★★) — 10분짜리 에디터 작업, 즉효
2. §1 실행 로그 발행 배선 — 코드 1~2시간, 타격감 절반이 여기서 나옴
3. §2 셰이크 BP 2개 제작 + 같은 지점 배선
4. §3 애님 노티파이 전수 확인
