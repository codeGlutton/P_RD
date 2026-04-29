/*****************************************************************//**
 * @file   UnitAttributeSet.h
 * @brief  SRPGUnit에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "AttributeSet.h"

#include "UnitAttributeSet.generated.h"

/**
 * @brief  SRPG Unit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UUnitAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UUnitAttributeSet();

	/* UAttributeSet 상속 */
public:
	void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MaxHP)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, HP)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, SkillPoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, DamagePoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, DefensePoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MovementPoint)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxHP;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData HP;

	/**
	 * @brief 스킬 시전 동안만 유지되는 스킬 포인트
	 * @details
	 * 공격 스킬 사용 시에는 최종 추가 공격력, 
	 * 방어 스킬 사용 시에는 최종 추가 방어력, 
	 * 이동 스킬 사용 시에는 최종 추가 이동력으로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData SkillPoint;
	/**
	 * @brief 피격 동안만 유지되는 피격 포인트
	 * @details
	 * 피격 시 실질적으로 들어오는 데미지로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData DamagePoint;

	// @brief 턴 동안만 유지되는 방어 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData DefensePoint;
	// @brief 턴 동안만 유지되는 움직임 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MovementPoint;
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

	/* UAttributeSet 상속 */
public:
	void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, Level)
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, MaxExp)
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, Exp)
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, Money)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Level;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxExp;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Exp;
	
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Money;
};