#include "UI/CharacterCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"

namespace
{
	constexpr float CardButtonAspect = 2.0f;

	const TCHAR* ResolveCharacterCardButtonTexturePath(EPlayerJobType JobType)
	{
		switch (JobType)
		{
		case EPlayerJobType::Knight:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_knight_symbol_button_1024x512.classselect_knight_symbol_button_1024x512");
		case EPlayerJobType::Archer:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_rogue_symbol_button_1024x512.classselect_rogue_symbol_button_1024x512");
		case EPlayerJobType::Mage:
			return TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_mage_symbol_button_1024x512.classselect_mage_symbol_button_1024x512");
		default:
			return nullptr;
		}
	}
}

/** @brief 카드 데이터는 부모가 주입하므로 생성자는 별도 에셋/목록 조회를 하지 않는다. */
UCharacterCardWidget::UCharacterCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

/** @brief 부모가 내려준 View 값과 선택 상태를 저장한 뒤 WBP 연출 이벤트로 밀어 넣는다. */
void UCharacterCardWidget::SetCharacterOption(const FFrontendCharacterOption& InOption, bool bSelected)
{
	mCharacterOption = InOption;
	mIsSelected = bSelected;
	SyncCard();
}

/** @brief 후보 값은 유지하고 선택 연출 상태만 다시 보낸다. */
void UCharacterCardWidget::SetSelected(bool bSelected)
{
	mIsSelected = bSelected;
	SyncCard();
}

/** @brief 카드 버튼 바인딩을 검증하고 클릭 이벤트를 이 카드의 안정 index 이벤트로 연결한다. */
void UCharacterCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();

	if (CardButton != nullptr)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	SyncCard();
}

/** @brief 위젯 재사용/화면 이탈 때 OnClicked 델리게이트를 제거한다. */
void UCharacterCardWidget::NativeDestruct()
{
	if (CardButton != nullptr)
	{
		CardButton->OnClicked.RemoveDynamic(this, &UCharacterCardWidget::HandleCardButtonClicked);
	}

	Super::NativeDestruct();
}

/** @brief 현재 View 값과 선택 상태를 BP 카드 연출에 전달한다. */
void UCharacterCardWidget::SyncCard()
{
	if (CardButton != nullptr)
	{
		// 잠긴 캐릭터도 눌러 상세/잠금 사유를 보여준다. 실제 확정 가능 여부는 선택 화면의 mSelectable 검증이 막는다.
		CardButton->SetIsEnabled(true);
	}

	ApplyCardButtonArt();
	BP_OnCharacterOptionChanged(mCharacterOption, mIsSelected);
}

void UCharacterCardWidget::ApplyCardButtonArt()
{
	if (CardButton == nullptr)
	{
		return;
	}

	const TCHAR* TexturePath = ResolveCharacterCardButtonTexturePath(mCharacterOption.mJobType);
	if (TexturePath == nullptr)
	{
		return;
	}

	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePath);
	if (Texture == nullptr || Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: failed to load card button texture: %s"), TexturePath);
		return;
	}

	FitRootSizeBoxToButtonAspect();

	CardButton->SetColorAndOpacity(FLinearColor::White);
	CardButton->SetBackgroundColor(FLinearColor::White);
	CardButton->SetRenderOpacity(1.0f);

	FSlateBrush ButtonBrush;
	ButtonBrush.DrawAs = ESlateBrushDrawType::Image;
	ButtonBrush.Tiling = ESlateBrushTileType::NoTile;
	ButtonBrush.Mirroring = ESlateBrushMirrorType::NoMirror;
	ButtonBrush.ImageSize = FVector2D(1024.0f, 512.0f);
	ButtonBrush.SetResourceObject(Texture);

	FButtonStyle ButtonStyle = CardButton->GetStyle();
	ButtonStyle.SetNormal(ButtonBrush);
	ButtonStyle.SetHovered(ButtonBrush);
	ButtonStyle.SetPressed(ButtonBrush);
	ButtonStyle.SetDisabled(ButtonBrush);
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f));
	ButtonStyle.SetNormalForeground(FSlateColor(FLinearColor::White));
	ButtonStyle.SetHoveredForeground(FSlateColor(FLinearColor::White));
	ButtonStyle.SetPressedForeground(FSlateColor(FLinearColor::White));
	ButtonStyle.SetDisabledForeground(FSlateColor(FLinearColor::White));
	CardButton->SetStyle(ButtonStyle);
}

void UCharacterCardWidget::FitRootSizeBoxToButtonAspect() const
{
	USizeBox* RootSizeBox = Cast<USizeBox>(GetRootWidget());
	if (RootSizeBox == nullptr && WidgetTree != nullptr)
	{
		WidgetTree->ForEachWidget([&RootSizeBox](UWidget* Widget)
		{
			if (RootSizeBox == nullptr)
			{
				RootSizeBox = Cast<USizeBox>(Widget);
			}
		});
	}
	if (RootSizeBox == nullptr)
	{
		return;
	}

	float TargetWidth = RootSizeBox->IsWidthOverride() ? RootSizeBox->GetWidthOverride() : 0.0f;
	float TargetHeight = RootSizeBox->IsHeightOverride() ? RootSizeBox->GetHeightOverride() : 0.0f;
	if (TargetWidth <= 0.0f && TargetHeight <= 0.0f)
	{
		return;
	}

	if (TargetHeight > 0.0f)
	{
		TargetWidth = TargetHeight * CardButtonAspect;
	}
	else
	{
		TargetHeight = TargetWidth / CardButtonAspect;
	}

	RootSizeBox->SetWidthOverride(TargetWidth);
	RootSizeBox->SetHeightOverride(TargetHeight);
	RootSizeBox->SetMinDesiredWidth(TargetWidth);
	RootSizeBox->SetMinDesiredHeight(TargetHeight);
}

/** @brief WBP_CharacterCard가 제공해야 하는 루트 버튼 바인딩 누락을 로그로 노출한다. */
void UCharacterCardWidget::ValidateDesignerBindings() const
{
	if (CardButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterCardWidget: WBP_CharacterCard requires CardButton."));
	}
}

/** @brief 클릭을 배열 위치가 아닌 GameMode가 부여한 CharacterOption.mIndex로 부모에게 전달한다. */
void UCharacterCardWidget::HandleCardButtonClicked()
{
	// 배열 위치가 아니라 GameMode가 내려준 안정적인 option index를 올려 보낸다.
	OnCharacterCardClicked.Broadcast(mCharacterOption.mIndex);
}
