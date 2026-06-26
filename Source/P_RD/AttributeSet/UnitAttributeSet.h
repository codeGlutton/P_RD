/*****************************************************************//**
 * @file   UnitAttributeSet.h
 * @brief  SRPGUnit에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "UnitAttributeSet.generated.h"

/**
 * @brief  SRPG Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UUnitAttributeSet : public UTacticalAttributeSet
{
	GENERATED_BODY()
	
public:
	UUnitAttributeSet();

	/* UTacticalAttributeSet 상속 */
public:
	// 미구현됨
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	// 미구현됨
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MaxHP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, HP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, SkillPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DamagePoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DefensePoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MovementPoint)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MaxHP;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HP;

	/**
	 * @brief 스킬 시전 동안만 유지되는 스킬 포인트
	 * @details
	 * 공격 스킬 사용 시에는 최종 추가 공격력, 
	 * 방어 스킬 사용 시에는 최종 추가 방어력, 
	 * 이동 스킬 사용 시에는 최종 추가 이동력으로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData SkillPoint;
	/**
	 * @brief 피격 동안만 유지되는 피격 포인트
	 * @details
	 * 피격 시 실질적으로 들어오는 데미지로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DamagePoint;

	// @brief 턴 동안만 유지되는 방어 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DefensePoint;
	// @brief 턴 동안만 유지되는 움직임 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MovementPoint;
};

/**
 * @brief  Player가 조작하는 SRPG Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UPlayerUnitAttributeSet : public UUnitAttributeSet
{
	GENERATED_BODY()
	
public:
	UPlayerUnitAttributeSet();

	/* UTacticalAttributeSet 상속 */
public:
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPlayerUnitAttributeSet, MaxExp)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPlayerUnitAttributeSet, Exp)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPlayerUnitAttributeSet, Money)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MaxExp;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Exp;
	
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Money;
};