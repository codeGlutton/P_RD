/*****************************************************************//**
 * @file   StaticSkillDataPropertyCustomization.cpp
 * @brief  StaticSkillData의 에디터 커스텀 디테일 패널 구현 소스
 * @author 모호재
 * @date   2026-07-24
 *********************************************************************/

#include "DataAsset/SkillData/StaticSkillDataPropertyCustomization.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "DataAsset/SkillData/StaticSkillData.h"

#define LOCTEXT_NAMESPACE "StaticSkillDataDetails"

TSharedRef<IDetailCustomization> FStaticSkillDataPropertyCustomization::MakeInstance()
{
	return MakeShareable(new FStaticSkillDataPropertyCustomization());
}

void FStaticSkillDataPropertyCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	/* 대상 에셋 포인터 구하기 */

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() == 0)
	{
		return;
	}

	mSelectedSkillDatas.Reset();
	for (const TWeakObjectPtr<UObject>& ObjectBeingCustomized : ObjectsBeingCustomized)
	{
		TWeakObjectPtr<UStaticSkillData> TargetSkillData = Cast<UStaticSkillData>(ObjectBeingCustomized.Get());
		if (TargetSkillData.IsValid() == false)
		{
			continue;
		}

		mSelectedSkillDatas.Add(TargetSkillData);
	}

	/* mDescription 프로퍼티 행 우측에 버튼 배치 커스터마이징 */

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("UI");
	TSharedRef<IPropertyHandle> DescriptionHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UStaticSkillData, mDescription));

	if (DescriptionHandle->IsValidHandle() == true)
	{
		IDetailPropertyRow& DescriptionPropertyRow = Category.AddProperty(DescriptionHandle);
		DescriptionPropertyRow.CustomWidget()
			.NameContent()
			[
				DescriptionHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(350.0f)
			.MaxDesiredWidth(600.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					DescriptionHandle->CreatePropertyValueWidget()
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				.VAlign(VAlign_Center)
				[
					SNew(SButton)
					.Text(LOCTEXT("GenerateDescription", "자동 생성"))
					.ToolTipText(LOCTEXT("GenerateDescriptionToolTip", "스킬 설정값에 맞춰 Description 텍스트를 자동 생성합니다."))
					.OnClicked(this, &FStaticSkillDataPropertyCustomization::OnGenerateDescription)
				]
			];
	}
}

FReply FStaticSkillDataPropertyCustomization::OnGenerateDescription() const
{
	for (const TWeakObjectPtr<UStaticSkillData>& SkillData : mSelectedSkillDatas)
	{
		if (SkillData.IsValid() == false)
		{
			continue;
		}

		SkillData->Modify();
		SkillData->mDescription = SkillData->MakeDescription();
		SkillData->MarkPackageDirty();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE