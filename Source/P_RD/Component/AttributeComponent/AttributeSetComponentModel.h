/*****************************************************************//**
 * @file   AttributeSetComponentModel.h
 * @brief  속성 컴포넌트 모델 정의 헤더
 * @author 김준형
 * @date   2026-06-19
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "AttributeSetComponentModel.generated.h"

// NOTE :	저희 GAS를 안쓰기로 해서 종속성을 다 빼려했는데, 요기서는 다시 쓰시는 것으로 보입니다.
//			월요일에 확실하게 결정을 하시죠.... 일단 여기서는 다 빼두었습니다.

UCLASS()
class P_RD_API UAttributeSetComponentModel : public UComponentModel
{
	GENERATED_BODY()

//public:	
//	UAttributeSetComponentModel();
//
//protected:
//	UPROPERTY(Category = GAS, VisibleAnywhere, meta = (DisplayName = "UnitAttributeSet"))
//	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;
//
//    TMap<FGameplayAttribute, FOnGameplayAttributeValueChange> DelegateMap;
//
//public:
//	virtual void Initialize() override;
//
//public:
//	virtual void BeginPlay() override;
//
//public:
//	/*
//    * @brief 해당하는 속성의 값을 가져온다.
//    */
//    float GetAttributeValue(FGameplayAttribute Attribute) const;
//
//    /*
//    * @brief 델리게이트를 찾아서 반환합니다.
//    */
//    FOnGameplayAttributeValueChange& GetGameplayAttributeValueChangeDelegate(FGameplayAttribute Attribute)
//    {
//        return DelegateMap.FindOrAdd(Attribute);
//    }
//    /*
//    * @brief 속성값을 변경합니다.
//    * 
//    * @details
//    * 변경하면서 델리게이트를 작동합니다.
//    * 어트리뷰트셋 수정에 제한을 두고 싶어 우선은 사용 자제 부탁드립니다.
//    * 
//    * @note
//    * 속도가 느리다고 함. 추후 변경 요망
//    */
//    void SetAttributeValue(FGameplayAttribute Attribute, float NewValue);
};
