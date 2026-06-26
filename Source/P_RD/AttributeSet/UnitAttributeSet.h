/*****************************************************************//**
 * @file   UnitAttributeSet.h
 * @brief  SRPGUnit에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "UnitAttributeSet.generated.h"

/**
 * @brief  SRPG Unit에 대한 Attribute Set 정의
 */

// NOTE :	PreAttributeChange
//			PostAttributeChange
//			작동 할 수 있도록 구현해야 함
UCLASS()
class P_RD_API UUnitAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	UUnitAttributeSet();

	/* UAttributeSet 상속 */
public:
	// 미구현됨
	void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	// 미구현됨
	void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MaxHP)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, HP)
	
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, AttackPoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, DefensePoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MovementPoint)

	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, DamagePoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, HealPoint)

	/* 단순 덧셈만 적용하는 영구적인 Attribute 값 */
protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxHP;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData HP;

	// @brief 타격 데미지 산출의 base가 되는 공격력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData AttackPoint;
	// @brief 턴 동안만 유지되는 방어 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData DefensePoint;
	// @brief 턴 동안만 유지되는 움직임 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MovementPoint;

	/* 복잡한 계산 이후에 실제 적용하는 일시적인 Attribute 값 */
protected:
	/**
	 * @brief 타격 동안만 유지되는 타격 포인트
	 * @details
	 * 타격 시 실질적으로 주는 데미지로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData DamagePoint;
	/**
	 * @brief 힐 동안만 유지되는 힐 포인트
	 * @details
	 * 힐 시 실질적으로 얻는 체력 변화로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData HealPoint;

	/**
	 * [NOTE] 상태이상은 그냥 Tag로 달자
	 */
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
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, MaxExp)
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, Exp)
	ATTRIBUTE_ACCESSORS(UPlayerUnitAttributeSet, Money)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxExp;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Exp;
	
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData Money;
};