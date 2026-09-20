/*****************************************************************//**
 * @file   OverlapGimmickModel.h
 * @brief  진입 트리거 기믹 모델 정의 헤더
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/Obstacle/Gimmick/GimmickModel.h"
#include "OverlapGimmickModel.generated.h"

/**
 * @brief  진입 트리거 기믹 모델 (부비트랩, 기절트랩, 밀치기발판 등)
 * @details 유닛 발밑에 깔리는 기믹. 길을 막지 않고, 자기 타일에 유닛이 들어오면 발동
 */
UCLASS(Blueprintable)
class P_RD_API UOverlapGimmickModel : public UGimmickModel
{
	GENERATED_BODY()

public:
	// @brief 장애물의 기본 성질(길 막음)을 해제: 유닛 아래 깔리는 Overlay 레이어로 바꾸고, 통행을 막지 않게 함
	UOverlapGimmickModel();

	/* IBoardCombatTarget 상속 */
public:
	// @brief 발밑 기믹은 스킬의 조준/피격 대상에서 제외
	bool IsTargetable() const override;

	/* UBoardActorModel 상속 */
public:
	// @brief 자기 타일에 발동 대상 레이어의 액터가 들어오면 발동
	void OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other) override;

protected:
	// @brief 발동 대상 레이어 (기본: 유닛)
	UPROPERTY(Category = "Gimmick", EditDefaultsOnly, BlueprintReadOnly, meta = (DisplayName = "TriggerLayerFlags", Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag"))
	int32 mTriggerLayerFlags = static_cast<int32>(ETileLayerFlag::Unit);
};
