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

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, RechargeMovement)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData RechargeMovement;
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