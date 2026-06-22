/*****************************************************************//**
 * @file   LevelAttributeSet.h
 * @brief  Player 레벨에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-05-16
 *********************************************************************/

#pragma once

#include "AttributSet/AttributeSetMinimal.h"
#include "AttributeSet.h"

#include "LevelAttributeSet.generated.h"

/**
 * @brief  Level에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API ULevelAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
	
public:
	ULevelAttributeSet();

public:
	ATTRIBUTE_ACCESSORS(ULevelAttributeSet, CommonSkillWeight)
	ATTRIBUTE_ACCESSORS(ULevelAttributeSet, RareSkillWeight)
	ATTRIBUTE_ACCESSORS(ULevelAttributeSet, EpicSkillWeight)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData CommonSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData RareSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FGameplayAttributeData EpicSkillWeight;
};

