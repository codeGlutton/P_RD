/*****************************************************************//**
 * @file   EnemyUnitModel.h
 * @brief  적 베이스 유닛 모델 정의 헤더
 * @author 모호재, 이문환
 * @date   2026-05-15
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/UnitModel.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h" // EMoveTendency
#include "EnemyUnitModel.generated.h"

class UUnitAttributeSet;

/**
 * @brief 적 베이스 유닛 모델
 */
UCLASS(abstract)
class P_RD_API UEnemyUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	UEnemyUnitModel();

	/* UUnitModel 상속 */
public:
	void PostInitializeComponentModels() override;

public:
	int32 GetBoardActorLevel() const override;

public:
	// @brief 난이도
	int32 GetDifficulty() const override;
	// @brief 플레이어유닛 여부
	bool IsPlayerUnitModel() const override { return false; }

	// @brief 이동 성향
	EMoveTendency GetMoveTendency() const;
	// @brief 이동포인트
	int32 GetMovePoint() const;

protected:
	// @brief 초기 스텟에 반영되는 난이도 수치
	// 스탯 커브의 레벨 축은 1부터 시작하고 초기화기가 (레벨-1) 인덱스로 조회하므로, 0(미설정)이면
	// 조회가 실패해 스탯이 전부 0으로 남는다 — 최소 유효 난이도 1을 기본값으로 둔다.
	UPROPERTY(Category = Enemy, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Difficulty"))
	int32 mDifficulty = 1;

	/**
	 * @brief 적 공통 AttributeSet (HP/방어 등).
	 * @details 속성 컴포넌트는 오너 모델의 자식 UObject 중 AttributeSet을 자동 수집하므로, 생성자에서
	 *          서브오브젝트로 만들어 두기만 하면 등록된다(플레이어와 동일 패턴). 이 서브오브젝트가 없으면
	 *          수집되는 세트가 0개라 스탯 커브 초기화가 무경고로 건너뛰어지고 모든 스탯이 0으로 남는다.
	 */
	UPROPERTY(Category = AttributeSet, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

	// @brief 이동 성향
	UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MoveTendency"))
	EMoveTendency mMoveTendency = EMoveTendency::HoldRange;

	// @brief 이동포인트
	UPROPERTY(Category = AI, VisibleAnywhere, BlueprintReadOnly, meta = (DisplayName = "MovePoint"))
	int32 mMovePoint = 5;
};
