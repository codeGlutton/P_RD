#include "Animation/Channel/BoardEventChannelDetailsCustomization.h"
#include "Animation/Channel/BoardEventChannel.h"

#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"

#define LOCTEXT_NAMESPACE "BoardEventChannelDetailsCustomization"

TSharedRef<IPropertyTypeCustomization> FBoardEventChannelDetailsCustomization::MakeInstance()
{
	return MakeShareable(new FBoardEventChannelDetailsCustomization());
}

void FBoardEventChannelDetailsCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	HeaderRow
		.NameContent()
		[
			PropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			PropertyHandle->CreatePropertyValueWidget()
		];
}

void FBoardEventChannelDetailsCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	uint32 NumChildren = 0;
	if (PropertyHandle->GetNumChildren(NumChildren) == FPropertyAccess::Success)
	{
		for (uint32 Index = 0; Index < NumChildren; ++Index)
		{
			TSharedPtr<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(Index);
			if (ChildHandle.IsValid() == true && ChildHandle->IsValidHandle() == true)
			{
				ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
