#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateTypes.h"
#include "UI/CharacterCardWidget.h"
#include "UI/CharacterSelectWidgetPrivate.h"
#include "UObject/SoftObjectPath.h"

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
	RefreshLocalizedTextCache();
	RefreshCharacterOptions();
	SetStatusText(mReadyStatusText);
}

/** @brief 타이틀 버튼 아트와 동일한 3상태 ButtonStyle을 런타임에 주입한다. */
void UCharacterSelectWidget::ApplyButtonStyles() const
{
	/*
	 * 선택 화면의 버튼도 타이틀 버튼과 같은 텍스처를 쓴다.
	 * 버튼 뒤에 별도 Image를 까는 방식이 아니라 ButtonStyle 자체에 3상태 브러시를 넣어 press 피드백을 유지한다.
	 */
	struct FButtonTexture { UButton* Button; const TCHAR* Path; };
	const FButtonTexture Targets[] = {
		{ mConfirmButton,    TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Start_DarkFantasy_Base.UI_Button_Start_DarkFantasy_Base") },
		{ mBackToMainButton, TEXT("/Game/SVN/OutSideAsset/AICreation/Title/UI_Button_Settings_DarkFantasy_Base.UI_Button_Settings_DarkFantasy_Base") },
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

		FSlateBrush NormalBrush;
		NormalBrush.DrawAs = ESlateBrushDrawType::Image;
		NormalBrush.ImageSize = FVector2D(static_cast<float>(Texture->GetSizeX()), static_cast<float>(Texture->GetSizeY()));
		NormalBrush.SetResourceObject(Texture);

		FSlateBrush HoveredBrush = NormalBrush;
		// [합의필요] hover/pressed 색 보정값은 임시 시각 튜닝이다. 버튼 상태별 전용 텍스처가 생기면 제거한다.
		HoveredBrush.TintColor = FSlateColor(FLinearColor(1.12f, 1.12f, 1.12f, 1.0f));
		FSlateBrush PressedBrush = NormalBrush;
		PressedBrush.TintColor = FSlateColor(FLinearColor(0.85f, 0.85f, 0.85f, 1.0f));

		FButtonStyle Style = Target.Button->GetStyle();
		Style.SetNormal(NormalBrush);
		Style.SetHovered(HoveredBrush);
		Style.SetPressed(PressedBrush);
		Style.SetDisabled(NormalBrush);
		Target.Button->SetStyle(Style);
	}
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
		mConfirmButtonText->SetText(InText);
	}
	if (mBackToMainButtonText != nullptr)
	{
		mBackToMainButtonText->SetText(mBackText);
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
