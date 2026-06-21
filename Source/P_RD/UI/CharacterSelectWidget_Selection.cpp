#include "UI/CharacterSelectWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameMode/FrontendGameMode.h"
#include "UI/CharacterSelectWidgetPrivate.h"

namespace
{
	constexpr float GClassSelectIllustrationAspect = 16.0f / 9.0f;

	void FitClassActionImageSlotToViewport(const UCharacterSelectWidget* Owner, UImage* Image)
	{
		if (Owner == nullptr || Image == nullptr)
		{
			return;
		}

		UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Image->Slot);
		if (CanvasSlot == nullptr)
		{
			return;
		}

		const UWorld* World = Owner->GetWorld();
		const UGameViewportClient* GameViewport = World != nullptr ? World->GetGameViewport() : nullptr;
		FVector2D ViewportSize = FVector2D::ZeroVector;
		if (GameViewport != nullptr)
		{
			GameViewport->GetViewportSize(OUT ViewportSize);
		}
		if (ViewportSize.X <= 0.0f || ViewportSize.Y <= 0.0f)
		{
			return;
		}

		const float ViewportAspect = ViewportSize.X / ViewportSize.Y;
		FVector2D TargetSize;
		if (ViewportAspect < GClassSelectIllustrationAspect)
		{
			TargetSize.Y = ViewportSize.Y;
			TargetSize.X = ViewportSize.Y * GClassSelectIllustrationAspect;
		}
		else
		{
			TargetSize.X = ViewportSize.X;
			TargetSize.Y = ViewportSize.X / GClassSelectIllustrationAspect;
		}

		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D::ZeroVector);
		CanvasSlot->SetSize(TargetSize);
	}
}

/** @brief GameMode에서 후보 View 목록을 다시 받아 이전 선택을 최대한 보존한다. */
void UCharacterSelectWidget::RefreshCharacterOptions()
{
	const int32 PreviousSelectedCharacterIndex = mSelectedCharacterIndex;
	mCharacterOptions.Reset();
	mSelectedPlayerUnitId = FPrimaryAssetId();
	mSelectedCharacterIndex = INDEX_NONE;

	AFrontendGameMode* FrontendGameMode = GetFrontendGameMode();
	if (FrontendGameMode == nullptr || !FrontendGameMode->GetCharacterOptions(OUT mCharacterOptions))
	{
		RebuildCharacterCards();
		ClearSelectedCharacter();
		SetStatusText(mNoCharacterStatusText);
		SetConfirmButtonText(mConfirmText);
		return;
	}

	const FFrontendCharacterOption* PreservedOption = mCharacterOptions.FindByPredicate([PreviousSelectedCharacterIndex](const FFrontendCharacterOption& Option)
	{
		return Option.mIndex == PreviousSelectedCharacterIndex;
	});
	const FFrontendCharacterOption* FirstOption = mCharacterOptions.IsEmpty() ? nullptr : &mCharacterOptions[0];
	const FFrontendCharacterOption* FirstEnabledOption = mCharacterOptions.FindByPredicate([](const FFrontendCharacterOption& Option)
	{
		return Option.mSelectable;
	});
	// 새로 받은 후보 목록에서도 이전 선택을 유지한다. 없으면 플레이 가능한 첫 후보, 그것도 없으면 첫 항목을 보여준다.
	const FFrontendCharacterOption* SelectedOption = PreservedOption != nullptr
		? PreservedOption
		: (FirstEnabledOption != nullptr ? FirstEnabledOption : FirstOption);

	if (SelectedOption != nullptr)
	{
		mSelectedCharacterIndex = SelectedOption->mIndex;
		mSelectedPlayerUnitId = SelectedOption->mSelectable ? SelectedOption->mPlayerUnitId : FPrimaryAssetId();
	}

	SetConfirmButtonText(mConfirmText);
	RebuildCharacterCards();
	SyncSelectedCharacter();
}

/** @brief 카드 클릭 index를 현재 선택으로 저장하되 잠긴 카드는 PlayerUnitId를 비워 확정을 막는다. */
void UCharacterSelectWidget::SelectCharacter(int32 CharacterIndex)
{
	if (mStartRequested)
	{
		return;
	}

	const FFrontendCharacterOption* Option = GetCharacterOption(CharacterIndex);
	if (Option == nullptr)
	{
		SetStatusText(mNoCharacterStatusText);
		return;
	}

	mSelectedCharacterIndex = Option->mIndex;
	// 잠긴 캐릭터도 상세 정보는 보여주되, 확정에 사용할 PlayerUnitId는 비워 Confirm을 막는다.
	mSelectedPlayerUnitId = Option->mSelectable ? Option->mPlayerUnitId : FPrimaryAssetId();
	SyncCharacterCards();
	SyncSelectedCharacter();
}

/** @brief 현재 선택 View 값을 상세 텍스트, 직업별 아트, BP 이벤트, Confirm 활성화에 반영한다. */
void UCharacterSelectWidget::SyncSelectedCharacter()
{
	const FFrontendCharacterOption* SelectedOption = GetSelectedCharacterOption();
	if (SelectedOption == nullptr)
	{
		ClearSelectedCharacter();
		return;
	}

	if (mSelectedCharacterNameText != nullptr)
	{
		mSelectedCharacterNameText->SetText(SelectedOption->mDisplayName);
	}
	if (mSelectedCharacterRoleText != nullptr)
	{
		mSelectedCharacterRoleText->SetText(SelectedOption->mRoleText);
	}
	if (mSelectedCharacterStatText != nullptr)
	{
		const bool bHasWBPStatValues = mMaxHPStatValueText != nullptr
			&& mDiceStatValueText != nullptr
			&& mGoldStatValueText != nullptr;
		mSelectedCharacterStatText->SetText(BuildCharacterStatText(*SelectedOption));
		mSelectedCharacterStatText->SetVisibility(bHasWBPStatValues
			? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (mMaxHPStatValueText != nullptr)
	{
		mMaxHPStatValueText->SetText(FText::AsNumber(SelectedOption->mMaxHP));
	}
	if (mDiceStatValueText != nullptr)
	{
		mDiceStatValueText->SetText(FText::AsNumber(SelectedOption->mDice));
	}
	if (mGoldStatValueText != nullptr)
	{
		mGoldStatValueText->SetText(FText::AsNumber(SelectedOption->mGold));
	}
	if (mStatRowBox != nullptr)
	{
		mStatRowBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (mSelectedCharacterDescriptionText != nullptr)
	{
		mSelectedCharacterDescriptionText->SetText(SelectedOption->mDescription);
	}
	if (mSelectedCharacterPortraitFallbackText != nullptr)
	{
		mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::Collapsed);
		mSelectedCharacterPortraitFallbackText->SetText(SelectedOption->mDisplayName);
	}

	SyncSelectedCharacterArt(SelectedOption->mJobType);
	BP_OnSelectedCharacterChanged(*SelectedOption);

	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(SelectedOption->mSelectable && !mStartRequested);
	}

	// 선택 상태 문구는 상세 패널 노이즈라 숨긴다. 로딩/실패 같은 흐름 상태만 SetStatusText로 노출한다.
	SetStatusText(FText::GetEmpty());
}

/** @brief 후보가 없거나 선택이 무효한 상태를 상세 패널 fallback으로 되돌린다. */
void UCharacterSelectWidget::ClearSelectedCharacter()
{
	SyncSelectedCharacterArt(EPlayerJobType::None);
	BP_OnSelectedCharacterCleared();

	if (mSelectedCharacterNameText != nullptr)
	{
		mSelectedCharacterNameText->SetText(mCharacterSelectText);
	}
	if (mSelectedCharacterRoleText != nullptr)
	{
		mSelectedCharacterRoleText->SetText(FText::GetEmpty());
	}
	if (mSelectedCharacterStatText != nullptr)
	{
		mSelectedCharacterStatText->SetText(FText::GetEmpty());
		mSelectedCharacterStatText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (mMaxHPStatValueText != nullptr)
	{
		mMaxHPStatValueText->SetText(FText::GetEmpty());
	}
	if (mDiceStatValueText != nullptr)
	{
		mDiceStatValueText->SetText(FText::GetEmpty());
	}
	if (mGoldStatValueText != nullptr)
	{
		mGoldStatValueText->SetText(FText::GetEmpty());
	}
	if (mStatRowBox != nullptr)
	{
		mStatRowBox->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mSelectedCharacterDescriptionText != nullptr)
	{
		mSelectedCharacterDescriptionText->SetText(mNoCharacterStatusText);
	}
	if (mSelectedCharacterPortraitFallbackText != nullptr)
	{
		mSelectedCharacterPortraitFallbackText->SetText(RDCharacterSelect::Text(TEXT("PortraitFallbackText")));
		mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (mConfirmButton != nullptr)
	{
		mConfirmButton->SetIsEnabled(false);
	}
}

/** @brief 선택 직업에 맞는 액션 일러스트만 표시하고 구형 Portrait 슬롯은 fallback 경로로 유지한다. */
void UCharacterSelectWidget::SyncSelectedCharacterArt(EPlayerJobType JobType)
{
	/*
	 * 직업별 액션 이미지는 WBP에 이미 배치돼 있지만 브러시가 비어 있다.
	 * 아트 원본은 팀 정책상 git이 아니라 SVN(Content/SVN)에만 있으므로, 타이틀 로고와 동일하게
	 * 런타임에 PNG를 로드해 각 이미지에 채우고(캐시), 선택된 직업의 이미지만 표시한다.
	 */
	const auto ApplyJobImage = [this](UImage* Image, EPlayerJobType ImageJob, EPlayerJobType SelectedJob)
	{
		if (Image == nullptr)
		{
			return;
		}

		if (ImageJob != SelectedJob)
		{
			Image->SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		FitClassActionImageSlotToViewport(this, Image);

		if (UTexture2D* Illustration = GetOrLoadJobIllustration(ImageJob))
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(
				static_cast<float>(Illustration->GetSizeX()),
				static_cast<float>(Illustration->GetSizeY()));
			Brush.SetResourceObject(Illustration);
			Image->SetBrush(Brush);
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image->SetVisibility(ESlateVisibility::Collapsed);
		}
	};

	ApplyJobImage(mKnightActionImage, EPlayerJobType::Knight, JobType);
	// [합의필요] 현재 WBP/아트 파일명은 Rogue지만 게임 enum은 Archer라 매핑을 코드에서 고정한다.
	ApplyJobImage(mRogueActionImage, EPlayerJobType::Archer, JobType);
	ApplyJobImage(mMageActionImage, EPlayerJobType::Mage, JobType);

	const bool bHasClassActionImages = mKnightActionImage != nullptr || mRogueActionImage != nullptr || mMageActionImage != nullptr;
	if (mSelectedCharacterPortraitImage == nullptr)
	{
		return;
	}

	// 직업별 액션 이미지를 쓰는 WBP에서는 공용 포트레이트 슬롯을 숨긴다. 구형 WBP fallback일 때만 포트레이트에 직접 채운다.
	if (bHasClassActionImages)
	{
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (UTexture2D* Illustration = GetOrLoadJobIllustration(JobType))
	{
		FSlateBrush PortraitBrush;
		PortraitBrush.DrawAs = ESlateBrushDrawType::Image;
		PortraitBrush.ImageSize = FVector2D(
			static_cast<float>(Illustration->GetSizeX()),
			static_cast<float>(Illustration->GetSizeY()));
		PortraitBrush.SetResourceObject(Illustration);
		mSelectedCharacterPortraitImage->SetBrush(PortraitBrush);
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::HitTestInvisible);

		if (mSelectedCharacterPortraitFallbackText != nullptr)
		{
			mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		mSelectedCharacterPortraitImage->SetVisibility(ESlateVisibility::Collapsed);
		if (mSelectedCharacterPortraitFallbackText != nullptr)
		{
			mSelectedCharacterPortraitFallbackText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

/** @brief 직업별 일러스트 SoftObject를 필요 시 동기 로드하고 UPROPERTY 캐시에 보관한다. */
UTexture2D* UCharacterSelectWidget::GetOrLoadJobIllustration(EPlayerJobType JobType)
{
	if (const TObjectPtr<UTexture2D>* Cached = mJobIllustrationCache.Find(JobType))
	{
		if (*Cached != nullptr)
		{
			return *Cached;
		}
	}

	const TSoftObjectPtr<UTexture2D>* Asset = mJobIllustrationAssets.Find(JobType);
	if (Asset == nullptr)
	{
		return nullptr;
	}

	// 타이틀 진입 직후 한 번만 일어나는 UI 로드라 동기 로드를 허용한다. 잦은 교체 UI가 되면 비동기 스트리밍으로 바꿔야 한다.
	UTexture2D* Loaded = Asset->LoadSynchronous();
	if (Loaded != nullptr)
	{
		mJobIllustrationCache.Add(JobType, Loaded);
	}
	return Loaded;
}

/** @brief 안정 index로 후보 View를 찾는다; 배열 위치 직접 접근을 피해 WBP 카드 순서 변경에 견딘다. */
const FFrontendCharacterOption* UCharacterSelectWidget::GetCharacterOption(int32 CharacterIndex) const
{
	return mCharacterOptions.FindByPredicate([CharacterIndex](const FFrontendCharacterOption& Option)
	{
		return Option.mIndex == CharacterIndex;
	});
}

/** @brief 현재 선택 index가 가리키는 후보 View를 가져온다. */
const FFrontendCharacterOption* UCharacterSelectWidget::GetSelectedCharacterOption() const
{
	return GetCharacterOption(mSelectedCharacterIndex);
}

/** @brief GameMode가 요약 문자열을 주지 않은 경우에만 숫자 필드로 표시 문구를 만든다. */
FText UCharacterSelectWidget::BuildCharacterStatText(const FFrontendCharacterOption& Option) const
{
	if (!Option.mStatSummary.IsEmpty())
	{
		return Option.mStatSummary;
	}

	return FText::Format(
		RDCharacterSelect::Text(TEXT("CharacterStatFormat")),
		FText::AsNumber(Option.mMaxHP),
		FText::AsNumber(Option.mDice),
		FText::AsNumber(Option.mGold));
}
