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

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, AttackPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DefensePoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MovementPoint)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DamagePoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, HealPoint)

	/* 단순 덧셈만 적용하는 영구적인 Attribute 값 */
protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MaxHP;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HP;

	// @brief 타격 데미지 산출의 base가 되는 공격력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData AttackPoint;
	// @brief 턴 동안만 유지되는 방어 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DefensePoint;
	// @brief 턴 동안만 유지되는 움직임 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MovementPoint;

	/* 복잡한 계산 이후에 실제 적용하는 일시적인 Attribute 값 */
protected:
	/**
	 * @brief 타격 동안만 유지되는 타격 포인트
	 * @details
	 * 타격 시 실질적으로 주는 데미지로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DamagePoint;
	/**
	 * @brief 힐 동안만 유지되는 힐 포인트
	 * @details
	 * 힐 시 실질적으로 얻는 체력 변화로 활용
	 */
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HealPoint;

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