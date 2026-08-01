# PRD: 스킬 범위 3단계 파이프라인 (조준범위 / 타겟범위 / 영향범위)

- 작성일: 2026-08-01
- 상태: D1~D6 확정, Phase 항목별 검토·구현 진행 중
- 근거: M의 「스킬 설정 건의안」 + 설계 논의 합의

---

## 1. 배경

현재 스킬 범위는 2단계(조준 → 영향)로 처리되며, 빔 스킬의 특수성이 영향범위 레이어에 잘못 얹혀 있다.

- `EEffectPattern::Beam`이 영향범위 패턴에 끼어 있어, `GetEffectTiles`가 `Caster` 파라미터를 받지만 실제로 쓰는 건 Beam뿐
- 빔은 "사정거리와 영향범위가 같아야 한다"는 단계 간 결합 제약이 발생
- 곡사(`mIsIndirect`)와 관통(`mIsPenetration`)이 서로 반대 극성의 불리언이라 데이터 세팅 실수 여지

## 2. 목표

- 범위 판정을 3단계 파이프라인으로 분리: **조준범위 → 타겟범위 → 영향범위**
- 각 단계는 입력/출력으로만 연결, 서로의 내부 설정을 참조하지 않음
- 빔의 특수성을 코드 분기가 아닌 데이터 값(`타겟패턴 = LineToTarget`)으로 이동
- 차단 설정을 불리언 극성 대신 "막는 주체" 비트마스크로 통일

### 비목표

- 스킬 이펙트/연출 시스템 변경 (타격 타일 목록 산출까지만 다룸)
- 새로운 조준/영향 패턴 모양 추가 (Cross/Star/Square 유지)

## 3. 파이프라인 정의

| 단계 | 입력 | 출력 | 차단 설정 |
|---|---|---|---|
| 1. 조준범위 | 시전자 좌표, 패턴, 사거리 | 조준 가능 타일 목록 | AimBlockerMask |
| 2. 타겟범위 | (시전자, 조준타일), 타겟패턴 | 영향범위 시점 타일 목록 | 없음 |
| 3. 영향범위 | 각 시점 타일, 패턴, 크기 | 최종 영향 타일 목록 (합집합, 중복 제거) | EffectBlockerMask |

- 일반 스킬: 타겟패턴 `TargetOnly` → 시점 = 조준타일 하나 (기존과 동일)
- 빔 스킬: 타겟패턴 `LineToTarget` → 시점 = 시전자→조준타일 경로 전체

### 합의된 규칙

- **중복 제거**: 시점별 영향범위가 겹쳐도 유닛은 1회만 피격 (최종 타일 목록은 합집합)
- **차단 독립**: 조준 단계와 영향 단계가 각각 자기 BlockerMask를 가짐. 어떤 조합도 유효
  - 예: 조준=None(곡사) + 영향=Unit|Obstacle → 장애물 너머로 조준되지만 각 지점의 확산은 벽에 막힘
- **차단(통과)과 조준 대상 포함(도착)은 별개 축**: `mCanAimBoardActor`(도착)는 그대로 유지

## 4. 현재 코드 현황

| 대상 | 위치 | 내용 |
|---|---|---|
| `EAimPattern` | `Source/P_RD/SRPGFramework/SRPGFrameworkType.h:127` | Single/Cross/Star/Square |
| `EEffectPattern` | 동일 파일 `:139` | Single/Cross/Star/Square/**Beam** |
| `ETileLayerFlag` | `Source/P_RD/Actor/TileMap/TileLayer.h:18` | Overlay/Unit/Obstacle 비트마스크 (기존) |
| 스킬 데이터 | `Source/P_RD/DataAsset/SkillData/StaticSkillData.h:113-165` | AimLogic(`mAimPattern`, `mIsIndirect`, `mCanAimBoardActor`), EffectLogic(`mEffectPattern`, `mIsPenetration`) |
| `GetAimableTiles` | `Source/P_RD/Actor/TileMap/TileMapModel.cpp:755` | `bIndirect`로 시야 검사 온오프 |
| `GetEffectTiles` | 동일 파일 `:835` | `Caster` 파라미터는 Beam 전용, `bPenetrate` |
| Beam 구현 | 동일 파일 `:872-886` | **클릭 지점에서 너머로** Size칸 뻗음 (시전자→클릭 아님) |
| 스킬 래퍼 | `Source/P_RD/Component/SkillComponent/SkillComponentModel.cpp:645,660` | 스킬 데이터 → 맵 모델 호출 변환 |
| 스킬 실행 | 동일 파일 `:160` | `ActivateSkill`이 `mEffectTileIndexes` 채움 |
| UI 프리뷰 | `Source/P_RD/SRPGFramework/SRPGSkillBuildAction.cpp:402,420` | 조준/영향 하이라이트 |
| UI 모양 매핑 | `Source/P_RD/GameMode/CombatGameMode.cpp:64,81` | 패턴 → UI 모양 enum 변환 (Beam 케이스 있음) |
| 적 AI | `Source/P_RD/SRPGFramework/SRPGEnemyTurnPlanner.cpp:173` | `GetAimableTiles` 직접 호출 |
| 기본 공격 | `Source/P_RD/Combat/CombatUIAdapter.cpp:222` | `GetAimableTiles` 직접 호출 |
| 테스트 | `Source/P_RDTests/Actor/TileMap/TileMapModelTests.cpp`, `Source/P_RDTests/SRPGFramework/EnemyTurnPlannerTests.cpp` | 범위 함수 커버 |

## 5. 결정 필요 항목 (구현 전 확정)

### D1. BlockerMask 타입: `ETileLayerFlag` 재사용 vs `EBlockerMask` 신설

- `ETileLayerFlag`(Unit/Obstacle 비트)가 이미 있고, `HasLineOfSight`도 이미 레이어 필터를 받음 (`GetPushPath`에서 `ETileLayerFlag::Obstacle`로 사용 중)
- **결정: `ETileLayerFlag` 재사용.** `EBlockerMask` 신설 안 함 (Phase 1.2 불필요)

### D2. `ETargetPattern` 초기 구성

- **결정: `TargetOnly`(조준타일 하나), `LineToTarget`(시전자→조준타일 경로) 두 개로 시작**
- 경로에 시전자 자신 타일은 포함하지 않음 (자기 포함은 기존 `ETargetIndexFilter::IncludeSelfIndex` 축이 담당)

### D3. 빔 의미 변화 승인

- 현재: 클릭 지점을 시작으로 **너머로** Size칸 (`TileMapModel.cpp:880`)
- 신규: 시전자부터 클릭 지점**까지** (M 제안 슬라이드 14)
- **결정: 신규 의미 승인.** 현행 동작은 조준/영향 결합 제약 때문에 시전자 옆 타일에서 시작할 수밖에 없었던 임시 구조. 새 구조에서는 그 모순이 없으므로 시전자로부터 시작

### D4. 기존 DA 에셋 마이그레이션 방식

- `mIsIndirect`/`mIsPenetration` 제거 시 기존 에셋 값 소실. 에셋은 SVN이라 팀 전체 영향
- **결정: 프로퍼티 즉시 교체.** 코드 작업 완료 후 DA 마이그레이션을 별도 작업으로 진행 (SVN 절차 준수)
- 마이그레이션 절차: 리뷰용 폴더(구버전 코드, 완전 분리 유지)에서 UE를 띄워 원본 값 참조, 현재 폴더 UE에서 수정. 구버전 에디터에서는 저장 금지
  - 스킬 DA는 **git 관리**(`Content/BP/DataAsset/Skill`, Attack 13 + Spell 5 = 18개, SVN 아님) — 마이그레이션은 git 커밋으로, 리뷰용 폴더는 develop 체크아웃이면 충분
  - 옛 값은 새 에디터로 재저장하기 전까지 에셋 파일에 남아 있어 구버전 코드로 조회 가능
  - 대상: 곡사/관통이던 스킬 → 마스크 `None` 재설정, 빔 스킬 → `TargetPattern = LineToTarget`
  - 18개 규모라 수동 진행 (Python 덤프 스크립트 불필요)

### D5. 타격 순서 보장 여부

- **결정: 시전자 가까운 순으로 정렬 보존** (연출이 순서대로 타격 가능하도록)
- 최종 영향 타일은 합집합이지만, 시점 목록 자체의 순서는 보존

### D6. 유닛/장애물 차단 기본값

- **결정: 비관통이 기본.** 조준 = Unit|Obstacle (기존 직사와 동일), 영향 = Unit|Obstacle (기존 비관통과 동일)
- 유닛/장애물 세분화는 마스크가 지원하므로 필요 시 스킬별 데이터에서 조정

## 6. 작업 분해

각 항목은 독립적으로 컴파일 가능한 단위. 순서는 의존성 순.

### Phase 1. 타입 정의

- **1.1** ✅ `ETargetPattern` 추가 — `SRPGFrameworkType.h`
  - `TargetOnly`, `LineToTarget` + ToolTip
- **1.2** ~~`EBlockerMask` 추가~~ — D1에서 `ETileLayerFlag` 재사용으로 결정, 불필요

### Phase 2. TileMapModel API

- **2.1** ✅ `GetTargetTiles` 신규 — `TileMapModel.h/.cpp`
  - 시그니처: `(Caster, Target, ETargetPattern) → TArray<FTileIndex>`
  - `TargetOnly`: `[Target]` / `LineToTarget`: `RasterizeLine`(Bresenham) 재사용으로 시야 검사와 같은 직선 정의 공유, 시전자 제외, 가까운 순
  - 임의 방향(예: (2,1)) 경로도 래스터화가 처리 — 8방향 양자화 안 씀
  - 조준 타일은 어떤 패턴에서든 항상 포함 (`Caster == Target`이어도 `[Target]` 반환)
- **2.2** ✅ `GetAimableTiles` 차단 파라미터 교체 — `bIndirect: bool` → `ETileLayerFlag BlockerLayers`
  - 내부 `HasLineOfSight` 호출에 레이어 필터 전달, `bApplyLineOfSight` 조건 재정리
  - 호출부 3곳 동시 대응 (스킬 래퍼/적 AI는 `mIsIndirect` 임시 변환, 기본 공격은 `Obstacle|Unit` 리터럴)
- **2.3** ✅ `GetEffectTiles` 차단 파라미터 교체 — `bPenetrate: bool` → `ETileLayerFlag BlockerLayers`
  - `AppendBlockableRay` 멈춤 판정을 레이어 기반으로 교체 (`None`이면 조건식 자체가 관통)
  - 스킬 래퍼는 `mIsPenetration` 임시 변환, 기존 Beam 테스트 3곳은 컴파일 유지용 기계적 치환
- **2.4** ✅ `GetEffectTiles`에서 `Caster` 파라미터와 `Beam` 분기 제거, `EEffectPattern`에서 `Beam` 제거
  - 6.2(Beam 테스트 대체), 5.2 일부(UI 모양 매핑 Beam 케이스 제거)와 동시 수행. 소스 전체에서 Beam 참조 0건 확인

### Phase 3. 스킬 데이터

- **3.1** ✅ `StaticSkillData`에 TargetLogic 카테고리 신설 — `mTargetPattern` (기본 `TargetOnly`)
- **3.2** ✅ `mIsIndirect` → `mAimBlockerMask` 교체 (int32 + `Bitmask, BitmaskEnum` 메타, 기본값 `Obstacle|Unit`)
- **3.3** ✅ `mIsPenetration` → `mEffectBlockerMask` 교체 (동일)
  - 소비처 동시 대응: 스킬 래퍼 2곳 임시 변환 제거, 적 AI, UI 채움부(bool 계약 유지, 마스크==0으로 유도), 적 AI 테스트

### Phase 4. SkillComponentModel 파이프라인 조립

- **4.1** ✅ `GetEffectTiles` 래퍼를 3단계 조합으로 재작성
  - 타겟 수집 → 시점별 확산 → `AddUnique` 합집합 (순서 보존으로 D5 연출 순서 유지)
  - Beam 분기 제거(2.4) 전까지 맵 모델 호출에 `SelfIndex` 전달 유지
  - 두 래퍼의 죽은 변수(`AimableTiles`) 제거
- **4.2** ✅ `GetTargetTiles` 래퍼 추가 (UI 프리뷰/연출 순서용)
- **4.3** ✅ `GetAimableTiles` 래퍼 새 시그니처 대응 (2.2, 3.2에서 처리)
- **4.4** ✅ `FActiveSkillContext` 멤버 이름 정리 — `mTargetTileIndex` → `mAimedTileIndex`, 스킬 레이어 파라미터 `TargetIndex` → `AimedTileIndex` 통일
  - 맵 모델의 `Target` 파라미터는 기하 레이어 일반 의미로 유지, `mTargetTileIndexes`(모션 타격 대상)는 새 '타겟' 의미와 일치해 유지

### Phase 5. 호출부 대응

- **5.1** `SRPGSkillBuildAction` — 프리뷰 하이라이트가 새 파이프라인 결과를 반영하는지 확인 (래퍼 경유라 변경 없을 가능성)
- **5.2** `CombatGameMode` UI 모양 매핑 — ✅ `EEffectPattern::Beam` 케이스 제거 / (잔여) 타겟패턴 표시 필요 여부 확인
  - (잔여) UI 계약 구조체(`CombatUITypes.h:286-287`)의 `mIsIndirect`/`mIsPenetration` 미러 필드와 채우는 곳도 마스크 기반으로 교체 — UI 팀과 계약 변경 협의 필요 (현재는 마스크==0으로 bool 유도해 계약 유지 중)
- **5.3** `SRPGEnemyTurnPlanner` — `GetAimableTiles` 새 시그니처 대응 (`mIsIndirect` 참조 교체)
- **5.4** `CombatUIAdapter` 기본 공격 — `GetAimableTiles` 새 시그니처 대응

### Phase 6. 테스트

- **6.1** ✅ `GetTargetTiles` 단위 테스트 신규 — TargetOnly, 직선/대각 경로와 순서, 경로 위 점유 무관, 자기 타일 조준 (5케이스)
- **6.2** ✅ 기존 Beam 테스트 대체 — 옛 "빔이 점유 칸에서 멈춤"은 조준(스윕) 단계 소관으로 이동, 영향범위 마스크 검증으로 재구성
- **6.3** ✅ BlockerMask 확산 테스트 — 차단 멈춤 / None 관통 / 점유 중심에서 확산 계속 (3케이스)
- **6.4** `EnemyTurnPlannerTests` 대응 — 스킬 데이터 세팅(`mIsIndirect`/`mIsPenetration`, `:84,87`)을 새 마스크 프로퍼티로 교체

## 7. 리스크

- **DA 에셋 호환성**: 프로퍼티 즉시 교체로 기존 스킬 에셋 값 소실 — 코드 작업 완료 후 DA 마이그레이션 별도 진행 (D4 확정). 에셋 작업은 SVN 절차 준수 필요
- **빔 수치 재조정**: 빔 의미 변화(D3 승인)로 기존 빔 스킬 에셋의 사거리/크기 수치 재조정 필요할 수 있음 — DA 마이그레이션 때 함께 처리
- **UI 계약**: `UI_API_CONTRACT.md` 등 UI 쪽이 패턴 enum에 의존하면 함께 갱신 필요
