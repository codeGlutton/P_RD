#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
#include "UI/CharacterCardWidget.h"

namespace
{
	const FText CharacterSelectTemporaryStats = NSLOCTEXT("CharacterSelectWidget", "TemporaryStats", "HP 100 / Dice 1 / Money 0");
	const FText CharacterSelectLockedReason = NSLOCTEXT("CharacterSelectWidget", "TemporaryLockedReason", "Locked");

	/**
	 * @brief UI 확인용 임시 캐릭터 표시 값을 만듦
	 *
	 * @details
	 * 이번 브랜치는 타이틀/캐릭터 선택 UI 구조만 다룬다.
	 * 실제 캐릭터 목록 API, 플레이어 유닛 확정, 런 시작, 방 전환은 담당 파트의 책임이라 여기서 호출하지 않는다.
	 *
	 * 그래서 현재는 WBP 배치와 카드 동작을 확인할 수 있도록 표시용 값만 만든다.
	 * 실제 캐릭터 데이터 제공자가 준비되면 RefreshCharacterOptions()에서 이 임시 목록 생성부를 교체하면 된다.
	 *
	 * @param DisplayName 상세 영역과 카드가 참조할 캐릭터 이름
	 * @param RoleText FRONT, RANGE, SPELL 같은 역할 표시 문구
	 * @param Description 상세 영역에 보여줄 간단한 설명
	 * @param bSelectable Confirm 버튼을 켤 수 있는 캐릭터면 true
	 * @param DisabledReason 잠긴 캐릭터일 때 상세 영역에 보여줄 사유
	 * @return 캐릭터 선택 UI가 화면 표시용으로 사용할 값
	 */
	FFrontendCharacterOption MakeTemporaryCharacterOption(
		const FText& DisplayName,
		const FText& RoleText,
		const FText& Description,
		bool bSelectable,
		const FText& DisabledReason = FText::GetEmpty()
	)
	{
		FFrontendCharacterOption Option;
		Option.mDisplayName = DisplayName;
		Option.mRoleText = RoleText;
		Option.mDescription = Description;
		Option.mStatSummary = CharacterSelectTemporaryStats;
		Option.bSelectable = bSelectable;
		Option.mDisabledReason = DisabledReason;
		return Option;
	}
}

UCharacterSelectWidget::UCharacterSelectWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CharacterCardWidgetClass(nullptr)
	, mCharacterSelectStatusText(FText::GetEmpty())
	, mCharacterTransitionBlockedStatusText(NSLOCTEXT("CharacterSelectWidget", "CharacterTransitionBlockedStatusText", "Next screen is not connected yet"))
	, mCharacterUnavailableStatusText(NSLOCTEXT("CharacterSelectWidget", "CharacterUnavailableStatusText", "This character is not available yet"))
	, mCharacterDataMissingStatusText(NSLOCTEXT("CharacterSelectWidget", "CharacterDataMissingStatusText", "Character data is not ready"))
	, mConfirmButtonText(NSLOCTEXT("CharacterSelectWidget", "ConfirmText", "CONFIRM"))
	, mBackButtonText(NSLOCTEXT("CharacterSelectWidget", "BackText", "BACK"))
{
	SetVisibility(ESlateVisibility::Visible);
}

void UCharacterSelectWidget::OpenCharacterSelect()
{
	RefreshCharacterOptions();
	SyncSelectedCharacter();
	SetStatusText(mCharacterSelectStatusText);
}

void UCharacterSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ValidateDesignerBindings();

	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}

	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}

	RefreshCharacterOptions();
	SyncCharacterText();
	SetStatusText(mCharacterSelectStatusText);
}

void UCharacterSelectWidget::NativeDestruct()
{
	if (ConfirmButton != nullptr)
	{
		ConfirmButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleConfirmButtonClicked);
	}

	if (BackToMainButton != nullptr)
	{
		BackToMainButton->OnClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleBackToMainButtonClicked);
	}

	for (UCharacterCardWidget* CharacterCardWidget : mCharacterCardWidgets)
	{
		if (CharacterCardWidget != nullptr)
		{
			CharacterCardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		}
	}

	CancelPortraitLoad();

	Super::NativeDestruct();
}

void UCharacterSelectWidget::RefreshCharacterOptions()
{
	mCharacterOptions.Reset();

	mCharacterOptions.Add(MakeTemporaryCharacterOption(
		NSLOCTEXT("CharacterSelectWidget", "TemporaryWarriorName", "Warrior"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryWarriorRole", "FRONT"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryWarriorDescription", "Temporary selectable slot for checking the character select UI."),
		true
	));

	mCharacterOptions.Add(MakeTemporaryCharacterOption(
		NSLOCTEXT("CharacterSelectWidget", "TemporaryArcherName", "Archer"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryArcherRole", "RANGE"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryArcherDescription", "Temporary locked slot. Real character data will be connected later."),
		false,
		CharacterSelectLockedReason
	));

	mCharacterOptions.Add(MakeTemporaryCharacterOption(
		NSLOCTEXT("CharacterSelectWidget", "TemporaryMagicianName", "Magician"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryMagicianRole", "SPELL"),
		NSLOCTEXT("CharacterSelectWidget", "TemporaryMagicianDescription", "Temporary locked slot. Real character data will be connected later."),
		false,
		CharacterSelectLockedReason
	));

	if (mCharacterOptions.IsValidIndex(mSelectedCharacterIndex) == false)
	{
		mSelectedCharacterIndex = 0;
	}

	SyncCharacterText();
	RebuildCharacterCards();
}

void UCharacterSelectWidget::SyncCharacterText() const
{
	if (ConfirmButtonText != nullptr)
	{
		ConfirmButtonText->SetText(mConfirmButtonText);
	}

	if (BackToMainButtonText != nullptr)
	{
		BackToMainButtonText->SetText(mBackButtonText);
	}

	if (BackToMainButton != nullptr)
	{
		BackToMainButton->SetVisibility(ESlateVisibility::Visible);
		BackToMainButton->SetIsEnabled(true);
	}
}

void UCharacterSelectWidget::RebuildCharacterCards()
{
	if (CharacterCardContainer == nullptr || CharacterCardWidgetClass == nullptr)
	{
		return;
	}

	while (mCharacterCardWidgets.Num() > mCharacterOptions.Num())
	{
		UCharacterCardWidget* RemovedCharacterCardWidget = mCharacterCardWidgets.Pop(EAllowShrinking::No);
		if (RemovedCharacterCardWidget != nullptr)
		{
			RemovedCharacterCardWidget->OnCharacterCardClicked.RemoveDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
			CharacterCardContainer->RemoveChild(RemovedCharacterCardWidget);
		}
	}

	while (mCharacterCardWidgets.Num() < mCharacterOptions.Num())
	{
		UCharacterCardWidget* NewCharacterCardWidget = CreateWidget<UCharacterCardWidget>(this, CharacterCardWidgetClass);
		if (NewCharacterCardWidget == nullptr)
		{
			return;
		}

		NewCharacterCardWidget->OnCharacterCardClicked.AddUniqueDynamic(this, &UCharacterSelectWidget::HandleCharacterCardClicked);
		UPanelSlot* NewCardSlot = CharacterCardContainer->AddChild(NewCharacterCardWidget);
		if (UHorizontalBoxSlot* HorizontalCardSlot = Cast<UHorizontalBoxSlot>(NewCardSlot))
		{
			HorizontalCardSlot->SetPadding(FMargin(8.0f));
			HorizontalCardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			HorizontalCardSlot->SetHorizontalAlignment(HAlign_Center);
			HorizontalCardSlot->SetVerticalAlignment(VAlign_Center);
		}
		mCharacterCardWidgets.Add(NewCharacterCardWidget);
	}

	for (int32 CharacterIndex = 0; CharacterIndex < mCharacterOptions.Num(); ++CharacterIndex)
	{
		UCharacterCardWidget* CharacterCardWidget = mCharacterCardWidgets[CharacterIndex];
		if (CharacterCardWidget == nullptr)
		{
			continue;
		}

		CharacterCardWidget->SetCharacterOption(mCharacterOptions[CharacterIndex], CharacterIndex);
		CharacterCardWidget->SetSelected(CharacterIndex == mSelectedCharacterIndex);
	}
}

void UCharacterSelectWidget::SyncCharacterCards() const
{
	for (int32 CharacterIndex = 0; CharacterIndex < mCharacterCardWidgets.Num(); ++CharacterIndex)
	{
		UCharacterCardWidget* CharacterCardWidget = mCharacterCardWidgets[CharacterIndex];
		if (CharacterCardWidget != nullptr)
		{
			CharacterCardWidget->SetSelected(CharacterIndex == mSelectedCharacterIndex);
		}
	}
}

void UCharacterSelectWidget::SelectCharacter(int32 CharacterIndex)
{
	const FFrontendCharacterOption* CharacterOption = GetCharacterOption(CharacterIndex);
	if (CharacterOption == nullptr)
	{
		SetStatusText(mCharacterDataMissingStatusText);
		return;
	}

	mSelectedCharacterIndex = CharacterIndex;
	SyncSelectedCharacter();
	SyncCharacterCards();
	SetStatusText(CharacterOption->bSelectable == true ? mCharacterSelectStatusText : mCharacterUnavailableStatusText);
}

void UCharacterSelectWidget::SyncSelectedCharacter()
{
	const FFrontendCharacterOption* SelectedCharacter = GetCharacterOption(mSelectedCharacterIndex);
	if (SelectedCharacter == nullptr)
	{
		ClearSelectedCharacter();
		return;
	}

	if (SelectedCharacterNameText != nullptr)
	{
		SelectedCharacterNameText->SetText(SelectedCharacter->mDisplayName);
	}

	if (SelectedCharacterRoleText != nullptr)
	{
		SelectedCharacterRoleText->SetText(SelectedCharacter->mRoleText);
	}

	if (SelectedCharacterDescriptionText != nullptr)
	{
		SelectedCharacterDescriptionText->SetText(SelectedCharacter->mDescription);
	}

	if (SelectedCharacterStatText != nullptr)
	{
		SelectedCharacterStatText->SetText(SelectedCharacter->mStatSummary);
	}

	if (SelectedCharacterDisabledReasonText != nullptr)
	{
		SelectedCharacterDisabledReasonText->SetText(SelectedCharacter->mDisabledReason);
		SelectedCharacterDisabledReasonText->SetVisibility(SelectedCharacter->mDisabledReason.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	SetPortraitImage(SelectedCharacter->mPortrait);

	if (ConfirmButton != nullptr)
	{
		ConfirmButton->SetIsEnabled(SelectedCharacter->bSelectable);
	}
}

void UCharacterSelectWidget::ClearSelectedCharacter()
{
	CancelPortraitLoad();
	ClearPortraitImage();

	if (SelectedCharacterNameText != nullptr)
	{
		SelectedCharacterNameText->SetText(FText::GetEmpty());
	}

	if (SelectedCharacterRoleText != nullptr)
	{
		SelectedCharacterRoleText->SetText(FText::GetEmpty());
	}

	if (SelectedCharacterDescriptionText != nullptr)
	{
		SelectedCharacterDescriptionText->SetText(FText::GetEmpty());
	}

	if (SelectedCharacterStatText != nullptr)
	{
		SelectedCharacterStatText->SetText(FText::GetEmpty());
	}

	if (SelectedCharacterDisabledReasonText != nullptr)
	{
		SelectedCharacterDisabledReasonText->SetText(FText::GetEmpty());
		SelectedCharacterDisabledReasonText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ConfirmButton != nullptr)
	{
		ConfirmButton->SetIsEnabled(false);
	}
}

void UCharacterSelectWidget::SetPortraitImage(const TSoftObjectPtr<UTexture2D>& Portrait)
{
	if (SelectedCharacterPortraitImage == nullptr)
	{
		CancelPortraitLoad();
		return;
	}

	if (Portrait.IsNull() == true)
	{
		CancelPortraitLoad();
		ClearPortraitImage();
		return;
	}

	const FSoftObjectPath PortraitPath = Portrait.ToSoftObjectPath();
	if (Portrait.IsValid() == true)
	{
		CancelPortraitLoad();
		SelectedCharacterPortraitImage->SetBrushFromTexture(Portrait.Get());
		SelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	if (mPortraitLoadHandle.IsValid() == true && mPendingPortraitPath == PortraitPath)
	{
		ClearPortraitImage();
		return;
	}

	CancelPortraitLoad();
	ClearPortraitImage();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (AssetManager == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: AssetManager is not ready."));
		return;
	}

	mPendingPortraitPath = PortraitPath;
	mPortraitLoadHandle = AssetManager->GetStreamableManager().RequestAsyncLoad(
		PortraitPath,
		FStreamableDelegate::CreateUObject(this, &UCharacterSelectWidget::HandlePortraitLoaded, PortraitPath),
		FStreamableManager::DefaultAsyncLoadPriority,
		false,
		false,
		TEXT("CharacterSelectPortrait")
	);
}

void UCharacterSelectWidget::HandlePortraitLoaded(FSoftObjectPath PortraitPath)
{
	if (mPendingPortraitPath != PortraitPath)
	{
		return;
	}

	UTexture2D* PortraitTexture = Cast<UTexture2D>(PortraitPath.ResolveObject());
	if (SelectedCharacterPortraitImage != nullptr && PortraitTexture != nullptr)
	{
		SelectedCharacterPortraitImage->SetBrushFromTexture(PortraitTexture);
		SelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		ClearPortraitImage();
	}

	mPortraitLoadHandle.Reset();
	mPendingPortraitPath.Reset();
}

void UCharacterSelectWidget::ClearPortraitImage() const
{
	if (SelectedCharacterPortraitImage != nullptr)
	{
		SelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterSelectWidget::CancelPortraitLoad()
{
	if (mPortraitLoadHandle.IsValid() == true)
	{
		mPortraitLoadHandle->CancelHandle();
		mPortraitLoadHandle.Reset();
	}

	mPendingPortraitPath.Reset();
}

void UCharacterSelectWidget::SetStatusText(const FText& InText)
{
	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}

	OnStatusTextChanged.Broadcast(InText);
}

const FFrontendCharacterOption* UCharacterSelectWidget::GetCharacterOption(int32 CharacterIndex) const
{
	return mCharacterOptions.IsValidIndex(CharacterIndex) ? &mCharacterOptions[CharacterIndex] : nullptr;
}

void UCharacterSelectWidget::ValidateDesignerBindings() const
{
	if (CharacterCardContainer == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: CharacterCardContainer is not connected."));
	}

	if (CharacterCardWidgetClass == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: CharacterCardWidgetClass is not set."));
	}

	if (SelectedCharacterNameText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterNameText is not connected."));
	}

	if (SelectedCharacterDescriptionText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterDescriptionText is not connected."));
	}

	if (SelectedCharacterRoleText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterRoleText is not connected."));
	}

	if (SelectedCharacterStatText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterStatText is not connected."));
	}

	if (SelectedCharacterDisabledReasonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterDisabledReasonText is not connected."));
	}

	if (SelectedCharacterPortraitImage == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: SelectedCharacterPortraitImage is not connected."));
	}

	if (ConfirmButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: ConfirmButton is not connected."));
	}

	if (ConfirmButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: ConfirmButtonText is not connected."));
	}

	if (BackToMainButton == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: BackToMainButton is not connected."));
	}

	if (BackToMainButtonText == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("CharacterSelectWidget: BackToMainButtonText is not connected."));
	}

	if (StatusText != nullptr)
	{
		StatusText->SetText(FText::GetEmpty());
		StatusText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCharacterSelectWidget::HandleCharacterCardClicked(int32 CharacterIndex)
{
	SelectCharacter(CharacterIndex);
}

void UCharacterSelectWidget::HandleConfirmButtonClicked()
{
	const FFrontendCharacterOption* SelectedCharacter = GetCharacterOption(mSelectedCharacterIndex);
	if (SelectedCharacter == nullptr)
	{
		SetStatusText(mCharacterDataMissingStatusText);
		return;
	}

	if (SelectedCharacter->bSelectable == false)
	{
		SetStatusText(mCharacterUnavailableStatusText);
		return;
	}

	UE_LOG(LogRD, Log, TEXT("CharacterSelectWidget: Confirm is blocked in the UI-only branch. Next screen flow is not connected."));
	SetStatusText(mCharacterTransitionBlockedStatusText);
}

void UCharacterSelectWidget::HandleBackToMainButtonClicked()
{
	OnBackToMainRequested.Broadcast();
}
