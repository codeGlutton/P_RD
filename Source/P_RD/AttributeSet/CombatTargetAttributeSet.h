/*****************************************************************//**
 * @file   CombatTargetAttributeSet.h
 * @brief  CombatTarget에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "CombatTargetAttributeSet.generated.h"

/**
 * @brief  SRPG Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UCombatTargetAttributeSet : public UTacticalAttributeSet
{
	GENERATED_BODY()
	
public:
	UCombatTargetAttributeSet();

	/* UTacticalAttributeSet 상속 */
public:
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, MaxHP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, HP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, Defense)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, Movement)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, AttackFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, HealFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, DefenseFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, MovementFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UCombatTargetAttributeSet, CriticalFactor)

	/* Instant로 즉각 적용되는 Attribute 값 */
protected:
	// @brief 최대 체력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MaxHP;
	// @brief 현재 체력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HP;

	// @brief 이번 턴 동안 유지되는 방어 스택
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Defense;
	// @brief 이번 턴 동안 유지되는 행동력 스택 
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Movement;

	/* 특정 기간적으로 추가되는 반영 스텟들 */
protected:
	// @brief 추가 공격력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData AttackFactor;
	// @brief 추가 회복력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HealFactor;
	// @brief 추가 방어휙득력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DefenseFactor;
	// @brief 추가 행동력 휙득력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MovementFactor;
	// @brief 크리티컬 확률 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData CriticalFactor;
};

