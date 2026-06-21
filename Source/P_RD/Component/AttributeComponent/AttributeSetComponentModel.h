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

    TMap<FGameplayAttribute, FOnGameplayAttributeValueChange> DelegateMap;

public:
	virtual void Initialize() override;

public:
	virtual void BeginPlay() override;

public:
	/*
    * @brief 해당하는 속성의 값을 가져온다.
    */
    float GetAttributeValue(FGameplayAttribute Attribute) const;

    /*
    * @brief 델리게이트를 찾아서 반환합니다.
    */
    FOnGameplayAttributeValueChange& GetGameplayAttributeValueChangeDelegate(FGameplayAttribute Attribute)
    {
        return DelegateMap.FindOrAdd(Attribute);
    }
    /*
    * @brief 속성값을 변경합니다.
    * 
    * @details
    * 변경하면서 델리게이트를 작동합니다.
    * 어트리뷰트셋 수정에 제한을 두고 싶어 우선은 사용 자제 부탁드립니다.
    * 
    * @note
    * 속도가 느리다고 함. 추후 변경 요망
    */
    void SetAttributeValue(FGameplayAttribute Attribute, float NewValue);
};
