/*****************************************************************//**
 * @file   AttributeSetComponentModel.h
 * @brief  속성 컴포넌트 모델 정의 헤더
 * @author 김준형
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "GAS/GASMinimal.h"
#include "Component/ComponentModel.h"
#include "GAS/Attribute/UnitAttributeSet.h"
#include "AttributeSetComponentModel.generated.h"

UCLASS()
class P_RD_API UAttributeSetComponentModel : public UComponentModel
{
	GENERATED_BODY()

public:	
	UAttributeSetComponentModel();

protected:
	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

public:
	virtual void Initialize() override;

public:
	virtual void BeginPlay() override;

public:
	UUnitAttributeSet* GetUnitAttributeSet() const;
};
