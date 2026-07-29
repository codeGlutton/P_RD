/*****************************************************************//**
 * @file   PartyAttributeSet.h
 * @brief  Party에 대한 Attribute Set 정의 헤더
 * @author 모호재
 * @date   2026-07-27
 *********************************************************************/

#pragma once

#include "AttributeSet/AttributeSetMinimal.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "PartyAttributeSet.generated.h"

/**
 * @brief  Party에 대한 Attribute Set 정의
 */
UCLASS()
class P_RD_API UPartyAttributeSet : public UTacticalAttributeSet
{
	GENERATED_BODY()
	
public:
	TACTICAL_ATTRIBUTE_ACCESSORS_BASIC(UPartyAttributeSet, Money)

public:
	static const FName KeyName;

protected:
	// @brief 최대 체력
	UPROPERTY(Category = Attribute, EditAnywhere, BlueprintReadWrite)
	FTacticalAttributeData Money;
};

