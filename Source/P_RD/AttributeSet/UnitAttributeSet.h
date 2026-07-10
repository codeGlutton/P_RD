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
	void PreAttributeChange(const FTacticalAttribute& Attribute, float& NewValue) override;
	void PostAttributeChange(const FTacticalAttribute& Attribute, float OldValue, float NewValue) override;

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MaxHP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, HP)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, Defense)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, Movement)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, AttackPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, HealPoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DefensePoint)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MovementPoint)

	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, AttackFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, HealFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, DefenseFactor)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UUnitAttributeSet, MovementFactor)

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
	// @brief 이번 턴 동안 유지되는 움직임 스택
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Movement;

	/* 임시적으로 패시브들이 스냅샷 참고하라고 넣어두는 기본 값 */
protected:
	// @brief 스킬 모션 동안만 유지되는 기본 공격 포인트 (스킬 기본 공격량과 주사위 계수 합산 값)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData AttackPoint;
	// @brief 스킬 모션 동안만 유지되는 기본 회복 포인트 (스킬 기본 회복량과 주사위 계수 합산 값)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData HealPoint;
	// @brief 스킬 모션 동안만 유지되는 기본 방어 획득 포인트 (스킬 기본 방어 획득량과 주사위 계수 합산 값)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData DefensePoint;
	// @brief 스킬 모션 동안만 유지되는 기본 움직임 획득 포인트 (스킬 기본 움직임 획득량과 주사위 계수 합산 값)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MovementPoint;

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
	// @brief 추가 이동휙득력 값 (ex 버프, 장비, 특정 기간 동안의 패시브 반영)
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData MovementFactor;
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