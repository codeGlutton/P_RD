/*****************************************************************//**
 * @file   CombatViewAdapter.h
 * @brief  전투 유닛/메타/턴 표시값을 비GAS 소스에서 만들어 전투 뷰모델에 밀어넣는 어댑터
 * @date   2026-06-16
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex

#include "CombatViewAdapter.generated.h"

class UCombatViewModel;
class USRPGCombatSubsystem;
class URunPersistData;
class AUnit;
class ATileMap;

/**
 * @brief 전투 유닛 한 기의 비GAS 런타임 상태(HP/이동력/타일). 플레이어는 실제 액터, 적은 가상.
 *
 * @details
 * GAS 폐기 + UUnitData 미존재 단계의 임시 데이터 계층(proto-UUnitData).
 * 적 액터 스폰은 에셋 파이프라인 이슈로 크래시하므로, 적은 액터 없이 이 상태로만 둔다(가상).
 * HP바는 타일→월드 변환으로 위치를 잡고, BASIC/STEP/MOVE는 이 상태를 직접 바꾼다.
 */
struct FCombatUnitState
{
	int32 mId = INDEX_NONE;
	bool mIsPlayer = false;
	FTileIndex mTile;
	float mHP = 0.0f;
	float mMaxHP = 0.0f;
	int32 mMovePoint = 0;
	TWeakObjectPtr<AUnit> mActor;   // 실제 액터(플레이어). 가상 적은 null.
};

/**
 * @brief 전투 HUD의 유닛 HP바·상단 메타·턴 표시와 BASIC/STEP/MOVE 액션을 구동하는 임시 게임플레이 대역(비GAS).
 *
 * @details
 * 데이터 소스는 전부 비GAS. 플레이어는 SRPGCombatSubsystem이 스폰한 실제 유닛, 적은 타일 위 가상 유닛.
 * UI는 SetUnitViews/SetPlayerMeta/SetTurnView로 받은 표시값만 읽고, 액션은 Request 의도로만 보낸다.
 * UUnitData가 생기면 이 상태/액션을 거기로 흡수한다.
 */
UCLASS(BlueprintType)
class P_RD_API UCombatViewAdapter : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 전투 서브시스템/런 데이터로 유닛 상태(플레이어 실제 + 가상 적)를 구성한다. */
	void Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun);

	/** @brief 뷰모델에 연결하고 현재 표시값을 push한다. */
	void BindViewModel(UCombatViewModel* InViewModel);

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

private:
	const FCombatUnitState* FindPlayerState() const;
	FCombatUnitState* FindStateById(int32 UnitId);
	FVector TileToWorld(const FTileIndex& Tile) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCombatViewModel> mViewModel;

	UPROPERTY(Transient)
	TObjectPtr<USRPGCombatSubsystem> mCombat;

	int32 mPlayerLevel = 1;
	int32 mPlayerGold = 100;
	int32 mNextUnitId = 0;

	TArray<FCombatUnitState> mUnitStates;
};
