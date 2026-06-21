#include "Component/AttributeComponent/AttributeSetComponentModel.h"

UAttributeSetComponentModel::UAttributeSetComponentModel()
{

}

void UAttributeSetComponentModel::Initialize()
{
	Super::Initialize();
}

void UAttributeSetComponentModel::BeginPlay()
{
	Super::BeginPlay();
}

float UAttributeSetComponentModel::GetAttributeValue(FGameplayAttribute Attribute) const
{
    // 1. Attribute에서 Property 정보를 가져옵니다.
    FProperty* Property = Attribute.GetUProperty();

    // 2. CastField를 사용하여 FNumericProperty(숫자형 속성)로 캐스팅합니다.
    // GAS의 모든 Attribute는 기본적으로 숫자(float/double)이므로 이것이 정석입니다.
    const FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);

    if (NumericProperty)
    {
        // 3. 해당 객체(this)의 메모리 주소를 넘겨 값을 읽어옵니다.
        return NumericProperty->GetFloatingPointPropertyValue(this);
    }

    return 0.0f; // 속성을 찾을 수 없거나 숫자형이 아닐 경우
}

void UAttributeSetComponentModel::SetAttributeValue(FGameplayAttribute Attribute, float NewValue)
{
    FOnAttributeChangeData ChangeData;
    ChangeData.Attribute = Attribute;
    ChangeData.OldValue = GetAttributeValue(Attribute);     // 무거움
    ChangeData.NewValue = NewValue;

    // 1. Attribute에서 Property 정보를 가져옵니다.
    FProperty* Property = Attribute.GetUProperty();

    // 2. 숫자형 프로퍼티로 캐스팅합니다.
    FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property);

    if (NumericProperty)
    {
        // 3. SetFloatingPointPropertyValue를 사용하여 값을 설정합니다.
        // 첫 번째 인자(this)는 값을 넣을 객체(현재 AttributeSet)입니다.
        // 두 번째 인자(NewValue)는 설정할 값입니다.
        NumericProperty->SetFloatingPointPropertyValue(this, static_cast<double>(NewValue));

        GetGameplayAttributeValueChangeDelegate(Attribute).Broadcast(ChangeData);
    }
}