/*****************************************************************//**
 * @file   TacticalAttributePropertyDetails.cpp
 * @brief  FTacticalAttribute 속성 선택 드롭다운 커스터마이제이션
 * @author 이문환
 * @date   2026-09-01
 *********************************************************************/

#include "TacticalAttributePropertyDetails.h"
#include "TAS/AttributeSet/TacticalAttributeSet.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "SSearchableComboBox.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<IPropertyTypeCustomization> FTacticalAttributePropertyDetails::MakeInstance()
{
	return MakeShareable(new FTacticalAttributePropertyDetails());
}

void FTacticalAttributePropertyDetails::CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	mStructHandle = StructPropertyHandle;
	BuildOptions();

	HeaderRow
	.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(250.f)
	[
		SNew(SSearchableComboBox)
		.OptionsSource(&mOptions)
		.OnGenerateWidget_Lambda([](TSharedPtr<FString> Option)
		{
			return SNew(STextBlock).Text(FText::FromString(*Option));
		})
		.OnSelectionChanged(this, &FTacticalAttributePropertyDetails::OnSelectionChanged)
		.Content()
		[
			SNew(STextBlock)
			.Text(this, &FTacticalAttributePropertyDetails::GetCurrentText)
		]
	];
}

void FTacticalAttributePropertyDetails::CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	// 헤더의 드롭다운으로 충분해서 자식 필드는 노출 안 함
}

void FTacticalAttributePropertyDetails::BuildOptions()
{
	mOptions.Reset();
	mPropertyByOption.Reset();

	// 해제용 선택지
	mOptions.Add(MakeShared<FString>(TEXT("None")));

	TArray<FProperty*> Properties;
	FTacticalAttribute::GetAllAttributeProperties(Properties);
	for (FProperty* Property : Properties)
	{
		const UStruct* Owner = Property->GetOwnerStruct();
		FString Option = FString::Printf(TEXT("%s.%s"), *GetNameSafe(Owner), *Property->GetName());
		mPropertyByOption.Add(Option, Property);
		mOptions.Add(MakeShared<FString>(MoveTemp(Option)));
	}
}

FText FTacticalAttributePropertyDetails::GetCurrentText() const
{
	if (mStructHandle.IsValid() == false)
	{
		return FText::GetEmpty();
	}

	// 다중 선택 편집까지 고려해 첫 값 기준으로 표시
	FText Result = FText::FromString(TEXT("None"));
	mStructHandle->EnumerateConstRawData([&Result](const void* RawData, const int32 DataIndex, const int32 NumDatas)
	{
		const FTacticalAttribute* Attribute = static_cast<const FTacticalAttribute*>(RawData);
		if (Attribute != nullptr && Attribute->IsValid() == true)
		{
			Result = FText::FromString(FString::Printf(TEXT("%s.%s"), *GetNameSafe(Attribute->GetAttributeSetClass()), *Attribute->GetName()));
		}
		return false;
	});
	return Result;
}

void FTacticalAttributePropertyDetails::OnSelectionChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo)
{
	if (mStructHandle.IsValid() == false || NewValue.IsValid() == false)
	{
		return;
	}

	// "None"이면 nullptr로 해제
	FProperty* Selected = mPropertyByOption.FindRef(*NewValue);

	// 트랜잭션/더티 처리를 위해 핸들 통지로 감싸고 구조체에 직접 기록
	mStructHandle->NotifyPreChange();
	mStructHandle->EnumerateRawData([Selected](void* RawData, const int32 DataIndex, const int32 NumDatas)
	{
		if (RawData != nullptr)
		{
			static_cast<FTacticalAttribute*>(RawData)->SetUProperty(Selected);
		}
		return true;
	});
	mStructHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
	mStructHandle->NotifyFinishedChangingProperties();
}
