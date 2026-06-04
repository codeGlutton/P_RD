#include "UI/CharacterCardWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

namespace
{
	/**
	 * @brief 하단 아이콘 placeholder에 사용할 기본 색을 고름
	 *
	 * @details
	 * 실제 아이콘 텍스처가 아직 없기 때문에 캐릭터 순서에 따라 서로 다른 단색을 임시로 사용한다.
	 * 선택된 캐릭터는 밝게, 선택되지 않은 캐릭터는 어둡게 표시한다.
	 * 잠긴 캐릭터는 전체적으로 어둡게 두되, 선택된 잠김 캐릭터는 조금 더 밝게 해서
	 * "눌리긴 했지만 시작은 못 한다"는 상태를 구분할 수 있게 한다.
	 *
	 * 이 색은 "임시 아이콘 배경"일 뿐이다.
	 * 나중에 FFrontendCharacterOption::mIcon에 실제 텍스처를 넣으면 IconImage가 그 이미지를 표시한다.
	 *
	 * @param CharacterIndex 캐릭터 목록에서 이 아이콘이 가리키는 번호
	 * @param bSelectable 선택 가능한 캐릭터면 true
	 * @param bSelected 현재 선택된 캐릭터면 true
	 * @return 아이콘 배경에 적용할 색
	 */
	FLinearColor GetFallbackIconColor(int32 CharacterIndex, bool bSelectable, bool bSelected)
	{
		static const FLinearColor BaseColors[] =
		{
			FLinearColor(0.72f, 0.16f, 0.12f, 1.0f),
			FLinearColor(0.15f, 0.56f, 0.24f, 1.0f),
			FLinearColor(0.34f, 0.18f, 0.66f, 1.0f),
			FLinearColor(0.70f, 0.55f, 0.18f, 1.0f),
			FLinearColor(0.18f, 0.55f, 0.68f, 1.0f)
		};

		const int32 ColorIndex = FMath::Abs(CharacterIndex) % UE_ARRAY_COUNT(BaseColors);
		const FLinearColor BaseColor = BaseColors[ColorIndex];
		const float Brightness = bSelectable == false ? (bSelected ? 0.38f : 0.22f) : bSelected ? 1.0f : 0.55f;
		return FLinearColor(BaseColor.R * Brightness, BaseColor.G * Brightness, BaseColor.B * Brightness, 1.0f);
	}

	/**
	 * @brief 예전 카드형 WBP에 남아 있는 텍스트를 비우고 숨김
	 *
	 * @details
	 * 현재 캐릭터 후보는 하단 아이콘으로만 보여준다.
	 * 하지만 이전 WBP나 실험용 위젯에 NameText, RoleText 같은 필드가 남아 있을 수 있으므로
	 * 값이 있으면 화면에 보이지 않게 정리한다.
	 *
	 * @param TextBlock 숨길 TextBlock
	 */
	void HideLegacyText(UTextBlock* TextBlock)
	{
		if (TextBlock == nullptr)
		{
			return;
		}

		TextBlock->SetText(FText::GetEmpty());
		TextBlock->SetVisibility(ESlateVisibility::Collapsed);
	}

}

UCharacterCardWidget::UCharacterCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

void UCharacterCardWidget::SetCharacterOption(const FFrontendCharacterOption& InOption, int32 InCharacterIndex)
{
	mCharacterOption = InOption;
	mCharacterIndex = InCharacterIndex;

	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(true);
	}

	SyncText();
}

void UCharacterCardWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	SyncText();
}

void UCharacterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	if (CardButton != nullptr)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	SyncText();
}

void UCharacterCardWidget::NativeDestruct()
{
	if (CardButton != nullptr)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	Super::NativeDestruct();
}

void UCharacterCardWidget::SyncText() const
{
	HideLegacyText(NameText);
	HideLegacyText(RoleText);
	HideLegacyText(StatText);
	HideLegacyText(DescriptionText);
	HideLegacyText(StateText);

	if (CardButton != nullptr)
	{
		CardButton->SetIsEnabled(true);
	}

	if (IconBackground != nullptr)
	{
		IconBackground->SetBrushColor(GetFallbackIconColor(mCharacterIndex, mCharacterOption.bSelectable, bIsSelected));
	}

	if (IconImage == nullptr)
	{
		return;
	}

	UTexture2D* LoadedIcon = mCharacterOption.mIcon.IsValid() ? mCharacterOption.mIcon.Get() : nullptr;
	if (LoadedIcon != nullptr)
	{
		IconImage->SetBrushFromTexture(LoadedIcon);
		IconImage->SetColorAndOpacity(mCharacterOption.bSelectable ? FLinearColor::White : FLinearColor(0.35f, 0.35f, 0.35f, 0.8f));
		IconImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		IconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterCardWidget::ValidateDesignerBindings() const
{
	if (CardButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: CardButton is not connected."));
	}

	if (IconBackground == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: IconBackground is not connected."));
	}

	if (IconImage == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: IconImage is not connected."));
	}
}

void UCharacterCardWidget::HandleCardButtonClicked()
{
	OnCharacterCardClicked.Broadcast(mCharacterIndex);
}
