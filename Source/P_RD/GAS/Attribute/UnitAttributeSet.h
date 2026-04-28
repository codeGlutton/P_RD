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
 * @brief  스킬 결과에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API USkillResultAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	ATTRIBUTE_ACCESSORS(USkillResultAttributeSet, SkillPoint)
	ATTRIBUTE_ACCESSORS(USkillResultAttributeSet, PreDefenseSkillPoint)
	ATTRIBUTE_ACCESSORS(USkillResultAttributeSet, PostDefenseSkillPoint)
	ATTRIBUTE_ACCESSORS(USkillResultAttributeSet, FinalSkillPoint)

protected:
	// @brief 버프 + 할당된 기본 스킬 포인트 값. 유저의 Skill 선택에 따라 할당됨
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData SkillPoint;
	// @brief 타격 스킬 포인트 값. 타격 전 패시브 Skill에 의한 효과를 모두 합연산 누적
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData PreDefenseSkillPoint;
	// @brief 실제 피격 스킬 포인트 값. 피격자의 방어력 만큼 감소
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData PostDefenseSkillPoint;
	// @brief 최종 적용 스킬 포인트 값. 타격, 피격 후 패시브 Skill에 의한 효과를 Max, Min으로 적용
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData FinalSkillPoint;
};

/**
 * @brief  SRPGUnit에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UUnitAttributeSet : public USkillResultAttributeSet
{
	GENERATED_BODY()
	
public:
	UUnitAttributeSet();

	/* UAttributeSet 상속 */
public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

public:
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MaxHP)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, HP)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, DefensePoint)
	ATTRIBUTE_ACCESSORS(UUnitAttributeSet, MovementPoint)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MaxHP;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData HP;

	// @brief 턴 동안만 유지되는 방어 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData DefensePoint;
	// @brief 턴 동안만 유지되는 움직임 포인트
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData MovementPoint;
};
