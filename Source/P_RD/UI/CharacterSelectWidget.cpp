#include "UI/CharacterSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"
#include "UI/CharacterCardWidget.h"
#include "UI/CharacterSelectWidgetPrivate.h"
#include "UObject/SoftObjectPath.h"

namespace
{
	void FitButtonSlotToTextureAspect(UButton* Button, const UTexture2D* Texture)
	{
		if (Button == nullptr || Texture == nullptr || Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
		{
			return;
		}

		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot);
		if (CanvasSlot == nullptr)
		{
			return;
		}

		FVector2D CurrentSize = CanvasSlot->GetSize();
		if (CurrentSize.X <= 0.0f && CurrentSize.Y <= 0.0f)
		{
			return;
		}

		const float TextureAspect = static_cast<float>(Texture->GetSizeX()) / static_cast<float>(Texture->GetSizeY());
		if (CurrentSize.X > 0.0f)
		{
			CurrentSize.Y = CurrentSize.X / TextureAspect;
		}
		else
		{
			CurrentSize.X = CurrentSize.Y * TextureAspect;
		}

		CanvasSlot->SetSize(CurrentSize);
	}

	void FitButtonParentSizeBoxToTextureAspect(UButton* Button, const UTexture2D* Texture)
	{
		if (Button == nullptr || Texture == nullptr || Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
		{
			return;
		}

		USizeBox* ParentSizeBox = Cast<USizeBox>(Button->GetParent());
		if (ParentSizeBox == nullptr)
		{
			return;
		}

		const float TextureAspect = static_cast<float>(Texture->GetSizeX()) / static_cast<float>(Texture->GetSizeY());
		if (ParentSizeBox->IsHeightOverride() && ParentSizeBox->GetHeightOverride() > 0.0f)
		{
			ParentSizeBox->SetWidthOverride(ParentSizeBox->GetHeightOverride() * TextureAspect);
		}
		else if (ParentSizeBox->IsWidthOverride() && ParentSizeBox->GetWidthOverride() > 0.0f)
		{
			ParentSizeBox->SetHeightOverride(ParentSizeBox->GetWidthOverride() / TextureAspect);
		}
	}
}

/** @brief 선택 화면 기본 문구와 직업별 일러스트 경로 계약을 준비한다. */
UCharacterSelectWidget::UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RefreshLocalizedTextCache();
	SetVisibility(ESlateVisibility::Visible);

	// 직업별 일러스트 텍스처 기본값(SVN 임포트 uasset). 직업 enum(Archer)과 아트 이름(rogue)이 다를 수 있어 여기서 매핑.
	// [합의필요] 아트 파일명이 직업명과 다를 수 있는 계약은 AssetRegistry/DataAsset로 이동할지 결정이 필요하다.
	mJobIllustrationAssets.Add(EPlayerJobType::Knight, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_knight_action_illustration_1920x1080.classselect_knight_action_illustration_1920x1080"))));
	mJobIllustrationAssets.Add(EPlayerJobType::Archer, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_rogue_action_illustration_1920x1080.classselect_rogue_action_illustration_1920x1080"))));
	mJobIllustrationAssets.Add(EPlayerJobType::Mage, TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/SVN/OutSideAsset/AICreation/ClassSelect/classselect_mage_action_illustration_1920x1080.classselect_mage_action_illustration_1920x1080"))));
}

/** @brief 타이틀 START로 진입할 때 중복 시작 게이트를 풀고 후보/상태를 새로 맞춘다. */
void UCharacterSelectWidget::OpenCharacterSelect()
{
	mStartRequested = false;
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
	SetConfirmButtonText(mConfirmText);
	ApplyButtonStyles();
	HideBackgroundObscuringWidgets();
}

/** @brief GameMode 후보 목록이 바뀐 경우 현재 카드와 선택 상세를 다시 그린다. */
void UCharacterSelectWidget::RefreshCharacterOptionsFromGameMode()
{
	RefreshCharacterOptions();
}

/** @brief WBP 바인딩 검증 후 이벤트/스타일/후보 데이터를 현재 런타임 상태로 초기화한다. */
void UCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ValidateDesignerBindings();
	BindEvents();
	ApplyButtonStyles();
	HideBackgroundObscuringWidgets();
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
}

/** @brief 해상도 변화에 맞춰 선택 일러스트 cover-crop 배치를 보정한다. */
void UCharacterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	HideBackgroundObscuringWidgets();
	FitSelectedCharacterArtToViewport();
}

/** @brief 타이틀 버튼 아트와 동일한 3상태 ButtonStyle을 런타임에 주입한다. */
void UCharacterSelectWidget::ApplyButtonStyles() const
{
	/*
	 * 이 함수는 위젯을 생성하지 않고, WBP가 제공한 버튼 인스턴스의 스타일/비율만 보정한다.
	 * 버튼의 배치/텍스트/상호작용 대상은 WBP_CharacterSelect가 계속 소유한다.
	 * 모바일에서는 hover/pressed 틴트가 눌린 상태처럼 남아 보일 수 있어 동일한 브러시를 사용한다.
	 */
	struct FButtonTexture { UButton* Button; const TCHAR* Path; };
	const FButtonTexture Targets[] = {
		{ mConfirmButton,    TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Enter_ImageGen.UI_Button_Enter_ImageGen") },
		{ mBackToMainButton, TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Back_ImageGen.UI_Button_Back_ImageGen") },
	};

	for (const FButtonTexture& Target : Targets)
	{
		if (Target.Button == nullptr)
		{
			continue;
		}
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Target.Path);
		if (Texture == nullptr)
		{
			continue;
		}

		Target.Button->SetColorAndOpacity(FLinearColor::White);
		Target.Button->SetBackgroundColor(FLinearColor::White);
		Target.Button->SetRenderOpacity(1.0f);
		Target.Button->SetIsEnabled(true);
		FitButtonSlotToTextureAspect(Target.Button, Texture);
		FitButtonParentSizeBoxToTextureAspect(Target.Button, Texture);

		FSlateBrush NormalBrush;
		NormalBrush.DrawAs = ESlateBrushDrawType::Image;
		NormalBrush.ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
		NormalBrush.SetResourceObject(Texture);

		FButtonStyle Style = Target.Button->GetStyle();
		Style.SetNormal(NormalBrush);
		Style.SetHovered(NormalBrush);
		Style.SetPressed(NormalBrush);
		Style.SetDisabled(NormalBrush);
		Style.SetNormalPadding(FMargin(0.f));
		Style.SetPressedPadding(FMargin(0.f));
		Style.SetNormalForeground(FSlateColor(FLinearColor::White));
		Style.SetHoveredForeground(FSlateColor(FLinearColor::White));
		Style.SetPressedForeground(FSlateColor(FLinearColor::White));
		Style.SetDisabledForeground(FSlateColor(FLinearColor::White));
		Target.Button->SetStyle(Style);
	}
}

void UCharacterSelectWidget::HideBackgroundObscuringWidgets() const
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	const auto MakeTransparent = [](UWidget* Widget)
	{
		if (Widget == nullptr)
		{
			return;
		}

		Widget->SetRenderOpacity(1.0f);
		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			Border->SetBrushColor(FLinearColor::Transparent);
		}
		else if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetColorAndOpacity(FLinearColor::Transparent);
		}
	};

	if (UWidget* ScreenBackground = WidgetTree->FindWidget(TEXT("MobileScreenBackground")))
	{
		MakeTransparent(ScreenBackground);
		ScreenBackground->SetVisibility(ESlateVisibility::Collapsed);
	}

	MakeTransparent(WidgetTree->FindWidget(TEXT("MobilePortraitFill")));
}

/** @brief Construct에서 연결한 이벤트를 해제해 화면 재진입 시 중복 클릭 처리를 막는다. */
void UCharacterSelectWidget::NativeDestruct()
{
	UnbindEvents();
	Super::NativeDestruct();
}

/** @brief 선택 화면 전용 고정 문구를 캐시해 UI 갱신 함수들이 같은 Text 키를 공유하게 한다. */
void UCharacterSelectWidget::RefreshLocalizedTextCache()
{
	mConfirmText = RDCharacterSelect::Text(TEXT("ConfirmText"));
	mBackText = RDCharacterSelect::Text(TEXT("BackText"));
	mReadyStatusText = RDCharacterSelect::Text(TEXT("ReadyStatusText"));
	mLoadingStatusText = RDCharacterSelect::Text(TEXT("LoadingStatusText"));
	mFailedStatusText = RDCharacterSelect::Text(TEXT("FailedStatusText"));
	mNoCharacterStatusText = RDCharacterSelect::Text(TEXT("NoCharacterStatusText"));
	mCharacterSelectText = RDCharacterSelect::Text(TEXT("CharacterSelectText"));
}

/** @brief Confirm/Back 버튼 이벤트를 연결한다; 카드 이벤트는 카드 재빌드 시점에 따로 연결된다. */
void UCharacterSelectWidget::BindEvents()
{
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (mBackToMainButton != nullptr)
	{
		mBackToMainButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}
}

/** @brief 버튼과 현재 카드 델리게이트를 모두 풀어 재Construct/카드 재수집 시 잔류 호출을 없앤다. */
void UCharacterSelectWidget::UnbindEvents()
{
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}
	if (mBackToMainButton != nullptr)
	{
		mBackToMainButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}

	for (UCharacterCardWidget* CardWidget : mCharacterCardWidgets)
	{
		if (CardWidget != nullptr)
		{
			CardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		}
	}
}

/** @brief 선택 화면 안의 상태 텍스트와 외부 호환 이벤트를 동시에 갱신한다. */
void UCharacterSelectWidget::SetStatusText(const FText& InText)
{
	if (mCharacterStatusText != nullptr)
	{
		const bool bHideDefaultStatus = InText.IsEmptyOrWhitespace() || InText.EqualTo(mReadyStatusText);
		mCharacterStatusText->SetVisibility(bHideDefaultStatus ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		mCharacterStatusText->SetText(InText);
	}

	OnStatusTextChanged.Broadcast(InText);
}

/** @brief Confirm 버튼에는 현재 상태 문구를, Back 버튼에는 캐시된 기본 문구를 반영한다. */
void UCharacterSelectWidget::SetConfirmButtonText(const FText& InText) const
{
	if (mConfirmButtonText != nullptr)
	{
		mConfirmButtonText->SetText(FText::GetEmpty());
		mConfirmButtonText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mBackToMainButtonText != nullptr)
	{
		mBackToMainButtonText->SetText(FText::GetEmpty());
		mBackToMainButtonText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

/** @brief 필수 WBP 바인딩 누락을 런타임 로그로 보여주되 C++ fallback 위젯은 만들지 않는다. */
void UCharacterSelectWidget::ValidateDesignerBindings() const
{
	if (mCharacterCardContainer == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mCharacterCardContainer."));
	}
	if (mConfirmButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mConfirmButton."));
	}
	if (mBackToMainButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: WBP_CharacterSelect requires mBackToMainButton."));
	}
}
