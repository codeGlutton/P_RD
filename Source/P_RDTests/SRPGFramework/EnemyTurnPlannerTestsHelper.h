/*****************************************************************//**
 * @file   EnemyTurnPlannerTestsHelper.h
 * @brief  USRPGEnemyTurnPlanner 테스트용 Mock 유닛모델 정의 헤더
 * @author 이문환
 * @date   2026-07-01
 *
 * @details
 *  USRPGEnemyTurnPlanner::PlanTurn은 static 함수지만,
 *  UEnemyUnitModel, UPlayerUnitModel, UTileMapModel을 파라미터로 요구.
 *  파라미터로 넘길 최소한의 Mock 정의
 *
 *  Pre/PostInitializeComponentModels 함수는 빈 코드로 대체하고,
 *  이동포인트는 모델의 mMovePoint를 코드로 세팅해서 사용.
 *********************************************************************/

#pragma once

#include "CoreMinimal.h"
#include "Pawn/Enemy/EnemyUnitModel.h"
#include "Pawn/UnitModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "DataAsset/UnitSpawnData/StaticEnemyUnitSpawnData.h" // EMoveTendency, ESkillPriority
#include "Component/SkillComponent/UnitSkillComponentModel.h"
#include "EnemyTurnPlannerTestsHelper.generated.h"

/**
 * @brief 스킬 컴포넌트 Mock
 * @details
 * 쿨다운 이펙트를 실제로 거는 API가 없으므로, 표시된 슬롯을 사용 불가로 판정해 쿨다운을 흉내냄.
 * 플래너는 CanActiveSkill 관문만 보므로 이것으로 "쿨다운이면 차순위" 검증이 가능.
 */
UCLASS()
class UMockSkillComponentModel : public UUnitSkillComponentModel
{
	GENERATED_BODY()

public:
	// @brief 슬롯을 쿨다운 상태로 표시
	void SetForcedCooldown(int32 SkillIndex)
	{
		mForcedCooldownSlots.Add(SkillIndex);
	}

protected:
	bool CanActiveSkill_Internal(int32 SkillIndex) const override
	{
		return mForcedCooldownSlots.Contains(SkillIndex) == false && Super::CanActiveSkill_Internal(SkillIndex);
	}

private:
	// @brief 쿨다운으로 표시된 슬롯
	TSet<int32> mForcedCooldownSlots;
};

/**
 * @brief 적유닛 Mock
 * @details 이동 성향/스킬/이동력을 코드로 세팅할 수 있는 최소 UEnemyUnitModel.
 */
UCLASS()
class UMockEnemyUnitModel : public UEnemyUnitModel
{
	GENERATED_BODY()

public:
	UMockEnemyUnitModel()
	{
		// UnitAttributeSet은 이제 부모(UEnemyUnitModel) 생성자가 동일한 이름("UnitAttributeSet")으로 만든다.
		// 여기서 중복 생성/선언하면 UHT 섀도잉 에러 — 상속분을 그대로 사용한다.

		// 쿨다운 흉내용 스킬 컴포넌트: 부모가 만든 것과 별개로 두고 조회 함수만 이쪽으로 돌림
		mMockSkillCompModel = CreateDefaultSubobject<UMockSkillComponentModel>(TEXT("MockSkillComponentModel"));
	}

	/**
	 * 월드서브시스템 건너뛰기 (빈 코드)
	 */
	virtual void PreInitializeComponentModels() override
	{
	}
	virtual void PostInitializeComponentModels() override
	{
	}

	// @brief 플래너가 보는 스킬 컴포넌트를 Mock으로 교체
	USkillComponentModel* GetSkillComponentModel() const override
	{
		return mMockSkillCompModel;
	}

	// @brief 이동성향 설정
	void SetMoveTendency(EMoveTendency Tendency)
	{
		mMoveTendency = Tendency;
	}

	// @brief 슬롯 우선순위 설정 (배열이 짧으면 Normal로 채우며 늘림)
	void SetSkillPriority(int32 SkillSlot, ESkillPriority Priority)
	{
		while (mSkillPriorities.Num() <= SkillSlot)
		{
			mSkillPriorities.Add(ESkillPriority::Normal);
		}
		mSkillPriorities[SkillSlot] = Priority;
	}

	// @brief 슬롯을 쿨다운 상태로 표시
	void SetSkillCooldown(int32 SkillSlot)
	{
		mMockSkillCompModel->SetForcedCooldown(SkillSlot);
	}

private:
	UPROPERTY()
	TObjectPtr<UMockSkillComponentModel> mMockSkillCompModel;
};

/**
 * @brief 플레이어유닛 Mock
 * @details 플레이어의 타일 좌표만 필요하니까 UUnitModel 베이스로 제작.
 */
UCLASS()
class UMockPlayerUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	/**
	 * 월드서브시스템 건너뛰기 (빈 코드)
	 */
	virtual void PreInitializeComponentModels() override
	{
	}
	virtual void PostInitializeComponentModels() override
	{
	}
	
	virtual int32 GetDifficulty() const override
	{
		return 0;
	}
	
	virtual bool IsPlayerUnitModel() const override
	{
		return true;
	}
};
