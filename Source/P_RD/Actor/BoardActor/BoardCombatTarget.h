/*****************************************************************//**
 * @file   BoardCombatTarget.h
 * @brief  SRPG 타일에 타격 가능한 모델 인터페이스 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "UObject/Interface.h"
#include "GenericTeamAgentInterface.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "BoardCombatTarget.generated.h"

class UAttributeSetComponentModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangeCombatTargetAliveState, bool /*IsAlive*/);

/**
 * @brief 타격 가능 보드 액터들에 대한 스냅샷
 */
UCLASS(BlueprintType)
class UBoardCombatTargetSnapshotData : public UObject
{
	GENERATED_BODY()

public:
	// @brief HP, Money, power 등의 "수치"
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	TMap<FTacticalAttribute, float> mAttributes;

	// @brief 밀치기, 약화(타격 피해량 25퍼 감소), 취약(피격 피해량 50퍼 증가) 등의 "발동 여부"
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, int32> mTags;

	// @brief 타일 위치
	UPROPERTY(Category = Targeting, EditAnywhere, BlueprintReadWrite)
	FTileTransform mTileTransform;
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
	UBoardCombatTargetSnapshotData* MakeSnapshotData() const;

public:
	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) = 0;
	virtual FGenericTeamId GetGenericTeamId() const = 0;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const UObject& Other) const;
};
