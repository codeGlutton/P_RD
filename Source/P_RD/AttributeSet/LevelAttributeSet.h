/*****************************************************************//**
 * @file   LevelAttributeSet.h
 * @brief  Player 레벨에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-05-16
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "LevelAttributeSet.generated.h"

/**
 * @brief  Level에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API ULevelAttributeSet : public UTacticalAttributeSet
{
	GENERATED_BODY()
	
public:
	ULevelAttributeSet();

public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, CommonSkillWeight)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, RareSkillWeight)
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(ULevelAttributeSet, EpicSkillWeight)

protected:
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData CommonSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData RareSkillWeight;
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData EpicSkillWeight;
};

