// @file CombatUIAdapter.h
// @brief 전투 유닛/메타/턴 표시값을 비GAS 소스에서 만들어 전투 뷰모델에 밀어넣는 어댑터
// @date 2026-06-16

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex
#include "Actor/TileMap/TileHighlight.h"       // ETileHighlightFlag

#include "CombatUIAdapter.generated.h"

class UCombatUIModel;
class USRPGCombatSubsystem;
class URunPersistData;
class UDicePoolModel;
class AUnit;
class ATileMap;
struct FPresentationBarrier;
struct FSRPGTurnContext;
enum class ECombatInputType : uint8;
enum class ESRPGTurnResult : uint8;

/** @brief 전투 유닛 한 기의 비GAS 런타임 상태입니다. 플레이어는 실제 액터, 적은 가상 상태로 둡니다. */
// GAS 폐기 + UUnitData 미존재 단계의 임시 데이터 계층(proto-UUnitData)이다.
// 적 액터 스폰이 아직 불안정해 적은 액터 없이 타일/HP 상태만 보관한다.
struct FCombatUnitState
{
	// 어댑터 내부에서만 쓰는 안정 id. 액터 포인터나 배열 위치를 UI payload로 노출하지 않기 위한 값이다.
	int32 mId = INDEX_NONE;
	// 플레이어만 실제 AUnit을 가진다. 가상 적은 타일/HP 상태만으로 HUD 검증을 진행한다.
	bool mIsPlayer = false;
	// 타겟 판정과 월드 위치 fallback의 단일 기준 타일.
	FTileIndex mTile;
	float mHP = 0.0f;
	float mMaxHP = 0.0f;
	int32 mMovePoint = 0;
	int32 mMaxMovePoint = 0;        // STEP으로 확보한 이동 총량(현재/최대 표시).
	TWeakObjectPtr<AUnit> mActor;   // 실제 액터(플레이어). 가상 적은 null.
};

/** @brief 전투 HUD 표시값과 BASIC/STEP/MOVE 액션을 임시로 연결하는 비GAS 어댑터입니다. */
// 플레이어는 SRPGCombatSubsystem이 스폰한 실제 유닛, 적은 타일 위 가상 유닛 상태를 사용한다.
// UI는 SetUnitUIs/SetPlayerMeta/SetTurnUI로 받은 표시값만 읽고, 액션은 Request 의도로만 보낸다.
// UUnitData가 생기면 이 임시 상태와 액션 처리는 그쪽으로 흡수될 예정이다.
UCLASS(BlueprintType)
class P_RD_API UCombatUIAdapter : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 전투 서브시스템/런 데이터로 유닛 상태(플레이어 실제 + 가상 적)를 구성한다. */
	void Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun);

	/** @brief 뷰모델에 연결하고 현재 표시값을 push한다. */
	void BindUIModel(UCombatUIModel* InUIModel);

	/** @brief 플레이어 다이스 컴포넌트를 연결한다(굴림 구동 + 쓴 주사위 잠금 + 뷰 push). */
	// CombatGameMode/HUD 배선이 채운다. 어댑터는 소유하지 않고 런타임 상태를 읽어 FDiceSlotUI로 변환한다.
	void SetDicePool(UDicePoolModel* InDiceComponent) { mDicePool = InDiceComponent; }

	/** @brief 현재 상태로 유닛/메타/턴 표시값을 다시 만들어 뷰모델에 push한다. */
	void PushAll();

	/* ── BASIC/STEP/MOVE 액션 (비GAS, 상태 직접 변경 후 push) ── */

	/** @brief 평타: 지정 유닛에게 Amount만큼 데미지. HP 0이면 가상 적 제거. */
	void ApplyBasicAttack(int32 TargetUnitId, int32 Amount);

	/** @brief STEP: 플레이어 이동력을 Amount만큼 올린다. */
	void ApplyStep(int32 Amount);

	/** @brief MOVE: 플레이어를 해당 타일로 이동(이동력 1 소모). 성공하면 true. */
	bool TryMovePlayer(const FTileIndex& TargetTile);

	/* ── 조회(world-touch 해석기가 타겟 판정에 사용) ── */

	/** @brief 해당 타일에 있는 유닛 id를 반환(없으면 INDEX_NONE). */
	int32 FindUnitIdAtTile(const FTileIndex& Tile) const;

	/** @brief 플레이어의 현재 타일을 반환. */
	FTileIndex GetPlayerTile() const;

	/** @brief 플레이어의 현재 이동력을 반환. */
	int32 GetPlayerMovePoint() const;

protected:
	virtual void BeginDestroy() override;

private:
	/** @brief UI 의도(스킬 선택/주사위 배치/이동/취소)를 받아 액션으로 처리한다. */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief 화면 터치 좌표를 라인 트레이스로 타일 판정해 타겟/이동/취소를 처리한다. */
	UFUNCTION()
	void HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	/** @brief 턴 종료 시 '쓴 주사위' 잠금을 해제한다(다음 턴 재사용 가능). Barrier는 붙잡지 않아 즉시 해제. */
	void HandleEndAnyTurn(TSharedPtr<FPresentationBarrier> Barrier, const FSRPGTurnContext& TurnContext, ESRPGTurnResult Result);

	/** @brief 뷰모델의 주사위 뷰에서 해당 index 굴림값을 읽는다(안 굴렸으면 0). */
	// ToggleDice는 UIModel에 push된 결과를 기준으로 판단해 UI가 보는 값과 액션 값이 갈라지지 않게 한다.
	int32 GetRolledDiceValue(int32 DiceIndex) const;

	/** @brief 다이스 컴포넌트를 굴리고(임시 난수 스트림) 그 결과를 뷰모델에 push한다. */
	void RollDice();

	/** @brief 컴포넌트의 현재 주사위 상태를 FDiceSlotUI[]로 변환해 UIModel->SetDiceUIs()로 push한다. */
	void PushDiceUIs() const;

	/** @brief 진행 중이던 스킬/주사위 선택을 초기화한다(바깥 탭 취소 등). */
	void ClearPendingAction();

	/** @brief 화면 좌표를 타일맵 평면에 투영해 타일 인덱스를 구한다. 타일맵 밖이면 false. */
	// UI는 스크린 좌표만 보내고, 카메라/타일맵 기준 해석은 어댑터/게임플레이 경계에서 수행한다.
	bool ResolveTileFromScreen(const FVector2D& ScreenPosition, FTileIndex& OutTile) const;

	/** @brief 한 타일만 지정 하이라이트(Aim=회색/Select=노랑/Effect=빨강)로 켠다(나머지는 끔). */
	void SetSingleTileHighlight(const FTileIndex& Tile, ETileHighlightFlag Flag) const;

	/** @brief 모든 하이라이트를 끈다. */
	void ClearAllHighlight() const;

	/** @brief 현재 상태 목록에서 플레이어 상태를 찾는다. 플레이어가 없으면 임시 어댑터는 조작을 무시한다. */
	const FCombatUnitState* FindPlayerState() const;

	/** @brief UIModel payload로 들어온 UnitId를 어댑터 상태로 되돌린다. */
	FCombatUnitState* FindStateById(int32 UnitId);

	/** @brief 가상 적처럼 Actor가 없는 상태의 HP바 위치를 타일맵 기준 월드 좌표로 보정한다. */
	FVector TileToWorld(const FTileIndex& Tile) const;

	/** @brief (시각 검증용 임시) 각 유닛 위치에 기본 도형 마커를 깐다 — 플레이어/적이 어디 있는지 보이게. 콜리전 없음(타일 트레이스 통과). */
	void RefreshUnitMarkers();

private:
	/** @brief UI 위젯들이 공유하는 표시/입력 계약 객체; 어댑터는 여기에 push하고 request를 구독한다. */
	UPROPERTY(Transient)
	TObjectPtr<UCombatUIModel> mUIModel;

	/** @brief 전투 월드 상태 접근점; 타일맵/유닛/턴 종료 이벤트만 읽는다. */
	UPROPERTY(Transient)
	TObjectPtr<USRPGCombatSubsystem> mCombat;

	/** @brief 플레이어가 가진 실제 런타임 주사위 묶음. 굴림/사용 잠금의 진짜 소스다. */
	UPROPERTY(Transient)
	TObjectPtr<UDicePoolModel> mDicePool;

	/** @brief OnEndAnyTurnUI(턴 종료) 구독 핸들. 해제/재빌드 때 Remove에 사용. */
	FDelegateHandle mEndTurnHandle;

	/** @brief 현재 스킬에 배치된 주사위 index(확정 시 '사용됨' 처리). -1이면 없음. */
	int32 mPendingDiceIndex = INDEX_NONE;

	/** @brief 메타 HUD 검증용 임시 레벨. 최종 소스는 RunPersistData/UUnitData 쪽으로 정리해야 한다. */
	int32 mPlayerLevel = 1;

	/** @brief 메타 HUD 검증용 임시 골드. [합의필요] 런 골드/전투 보상 소스와 연결 필요. */
	int32 mPlayerGold = 100;

	/** @brief 어댑터 내부 UnitId 발급 카운터; Build()마다 0부터 다시 시작한다. */
	int32 mNextUnitId = 0;

	/** @brief 현재 선택된 스킬 레일 index(BASIC=0, STEP=5). 없으면 INDEX_NONE. */
	int32 mSelectedSkillIndex = INDEX_NONE;

	/** @brief BASIC 평타가 타겟(적 탭)을 기다리는 중이면, 배치된 주사위 값. -1이면 대기 아님. */
	int32 mPendingAttackDamage = -1;

	/** @brief MOVE 모드(타일 탭 대기) 여부. */
	bool mMovePending = false;

	/** @brief STEP 단계 확정 대기 중인 주사위 값. -1이면 대기 아님. */
	int32 mPendingStepValue = -1;

	/** @brief STEP 확정 단계(0=회색, 1=노랑, 2=빨강+확정). */
	int32 mStepStage = 0;

	/** @brief STEP 확정 중인 본인 타일. */
	FTileIndex mStepTile;

	/** @brief BASIC 사거리(회색 Aim) 타일 목록. 이 안에서만 타깃 선택 가능. */
	TArray<FTileIndex> mAimTiles;

	/** @brief BASIC 1차 선택된 타깃 타일(노랑). 같은 칸 재탭 시 빨강+실행. Invalid면 아직 미선택. */
	FTileIndex mAttackTargetTile = FTileIndex::Invalid;

	/** @brief 플레이어 실제 상태와 가상 적 상태를 함께 담는 임시 전투 상태 테이블. */
	TArray<FCombatUnitState> mUnitStates;

	/** @brief (시각 검증용 임시) 유닛 위치 도형 마커 액터들. RefreshUnitMarkers가 매번 정리/재생성. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> mUnitMarkers;
};
