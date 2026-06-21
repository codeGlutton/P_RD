/*****************************************************************//**
 * @file   BoardCombatTarget.h
 * @brief  SRPG 타일에 타격 가능한 모델 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "GenericTeamAgentInterface.h"
#include "BoardCombatTarget.generated.h"

class UAttributeSetComponentModel;

USTRUCT(Blueprintable)
struct FBoardCombatTargetSnapshotData
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mMaxHP;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mHP;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mSkillPoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mDamagePoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mDefensePoint;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	float mMovementPoint;

	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	uint8 mBuffState;
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	uint8 mDebuffState;
};

UINTERFACE(MinimalAPI)
class UBoardCombatTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  SRPG 타일에 타격 가능한 모델
 */
class P_RD_API IBoardCombatTarget
{
	GENERATED_BODY()

public:
	/**
	 * 현재 타격 가능한 여부를 반환하는 함수
	 * @return 타격 가능 여부 
	 */
	virtual bool IsTargetable() const;
	/**
	 * 죽음 확인 함수
	 * @return 죽음 여부
	 */
	virtual bool IsDead() const;

public:
	/**
	 * 속성 컴포넌트를 반환하는 함수
	 * @return 보유한 속성 컴포넌트
	 */
	virtual UAttributeSetComponentModel* GetAttributeComponentModel() const = 0;
	/**
	 * 현재 스탯 스냅샷을 찍어 타겟 정보로 반환하는 함수
	 * @return 스냅샷 데이터
	 */
	virtual FBoardCombatTargetSnapshotData* MakeSnapshotData() const = 0;

public:
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) = 0;
	virtual FGenericTeamId GetGenericTeamId() const = 0;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const UObject& Other) const;
};
