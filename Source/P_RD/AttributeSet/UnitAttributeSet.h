/*****************************************************************//**
 * @file   UnitAttributeSet.h
 * @brief  Unit에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "AttributeSet/CombatTargetAttributeSet.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "UnitAttributeSet.generated.h"

/**
 * @brief  Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UUnitAttributeSet : public UCombatTargetAttributeSet
{
	GENERATED_BODY()

	/* UTacticalAttributeSet 상속 */
public:
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, ActionPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, SpeedPoint)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, ActionPointFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, SpeedPointFactor)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, RechargeActionPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, RechargeSpeedPoint)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, LastRechargedSpeedPoint)

	/* Instant로 즉각 적용되는 Attribute 값 */
protected:
	// @brief 이번 턴 동안 유지되는 행동력 스택 
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData ActionPoint;
	// @brief 자신의 턴마다 회복하는 행동력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData SpeedPoint;

	/* 특정 기간적으로 추가되는 반영 스텟들 */
protected:
	// @brief 추가 행동력 휙득력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData ActionPointFactor;
	// @brief 라운드마다 부여되는 속도 포인트 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData SpeedPointFactor;

	/* 특정 타이밍에만 쓰이는 스텟들 */
protected:
	// @brief 자신의 턴마다 회복하는 행동력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData RechargeActionPoint;
	// @brief 자신의 턴마다 회복하는 행동력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData RechargeSpeedPoint;

	/* 캐시 스텟들 */
protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData LastRechargedSpeedPoint;
};

/**
 * @brief  Enemy Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UEnemyUnitAttributeSet : public UUnitAttributeSet
{
	GENERATED_BODY()
};

/**
 * @brief  Player가 조작하는 Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UPlayerUnitAttributeSet : public UUnitAttributeSet
{
	GENERATED_BODY()
	
	/* UTacticalAttributeSet 상속 */
public:
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPlayerUnitAttributeSet, MaxExp)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPlayerUnitAttributeSet, Exp)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MaxExp;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Exp;
};