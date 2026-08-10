#include "UI/CombatResultOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "CombatResultOverlayWidget"

namespace
{
	FIntPoint NativeTextureSize(UTexture2D* Texture)
	{
		if (Texture == nullptr)
		{
			return FIntPoint::ZeroValue;
		}

		const FIntPoint ImportedSize = Texture->GetImportedSize();
		if (ImportedSize.X > 0 && ImportedSize.Y > 0)
		{
			return ImportedSize;
		}
		return FIntPoint(Texture->GetSizeX(), Texture->GetSizeY());
	}

	void SetAspectFitPortrait(UImage* Image, UTexture2D* Texture,
		const FVector2D BoundsPosition, const FVector2D BoundsSize)
	{
		if (Image == nullptr || Texture == nullptr)
		{
			return;
		}

		const FIntPoint TextureSize = NativeTextureSize(Texture);
		if (TextureSize.X <= 0 || TextureSize.Y <= 0)
		{
			return;
		}

		FSlateBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(TextureSize);
		Image->SetBrush(Brush);

		if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(Image->Slot))
		{
			const FVector2D NativeSize(TextureSize);
			const double UniformScale = FMath::Min(
				BoundsSize.X / NativeSize.X, BoundsSize.Y / NativeSize.Y);
			const FVector2D FittedSize = NativeSize * UniformScale;
			Slot->SetPosition(BoundsPosition + (BoundsSize - FittedSize) * 0.5f);
			Slot->SetSize(FittedSize);
		}
	}
}

UCombatResultOverlayWidget::UCombatResultOverlayWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	mViewportZOrder = 60;
	mRemoveFromParentOnClose = true;
}

void UCombatResultOverlayWidget::ShowVictoryReward(const FRewardUI& Reward, FSimpleDelegate ConfirmCallback)
{
	mMode = ECombatResultOverlayMode::VictoryReward;
	mReward = Reward;
	mTitleCallback = MoveTemp(ConfirmCallback);
	RefreshWidget();
}

void UCombatResultOverlayWidget::ShowDefeatResult(
	const FCombatResultUI& Result,
	FSimpleDelegate TitleCallback)
{
	mMode = ECombatResultOverlayMode::DefeatContinue;
	mReward = FRewardUI();
	mCombatResult = Result;
	mTitleCallback = MoveTemp(TitleCallback);
	BindButtons();
	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	UE_LOG(LogRD, Display, TEXT("Combat defeat WBP construct: title=%s"),
		mTitleButton != nullptr ? TEXT("bound") : TEXT("missing"));

	BindButtons();

	RefreshWidget();
}

void UCombatResultOverlayWidget::NativeDestruct()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
	Super::NativeDestruct();
}

void UCombatResultOverlayWidget::BindButtons()
{
	if (mTitleButton != nullptr)
	{
		mTitleButton->OnClicked.RemoveDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
		mTitleButton->OnClicked.AddUniqueDynamic(this, &UCombatResultOverlayWidget::HandleTitleClicked);
	}
}

void UCombatResultOverlayWidget::HandleTitleClicked()
{
	UE_LOG(LogRD, Display, TEXT("Combat defeat title button clicked."));
	FSimpleDelegate Callback = MoveTemp(mTitleCallback);
	mTitleCallback.Unbind();
	Callback.ExecuteIfBound();
}

void UCombatResultOverlayWidget::RefreshWidget()
{
	if (mMode != ECombatResultOverlayMode::DefeatContinue)
	{
		return;
	}

	// Defeat copy is always rendered in pure white. Only the foreground color is
	// normalized here; the WBP-authored dark outline and shadow remain unchanged.
	if (WidgetTree != nullptr)
	{
		WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (UTextBlock* Text = Cast<UTextBlock>(Widget))
			{
				Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			}
		});
	}

	// 확정 시안(0809 v2): 도달 지점·라운드는 한 줄, 나머지는 숫자만 크게.
	// 이름표(처치·획득 골드·생존)는 WBP 쪽 고정 글이 맡는다.
	if (mLocationText != nullptr)
	{
		mLocationText->SetText(FText::Format(
			LOCTEXT("LocationFormat", "{0} · {1} 라운드"),
			mCombatResult.mLocationName.IsEmpty()
				? LOCTEXT("UnknownLocation", "현재 전투 지역")
				: mCombatResult.mLocationName,
			FText::AsNumber(FMath::Max(1, mCombatResult.mRound))));
	}
	if (mRoundText != nullptr)
	{
		mRoundText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mEnemyText != nullptr)
	{
		mEnemyText->SetText(FText::AsNumber(mCombatResult.mDefeatedMonsterCount));
	}
	if (mGoldText != nullptr)
	{
		mGoldText->SetText(FText::AsNumber(mCombatResult.mGoldGained));
	}
	if (mExpText != nullptr)
	{
		mExpText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UTextBlock* Survivor = Cast<UTextBlock>(
		GetWidgetFromName(TEXT("DefeatSurvivorValue"))))
	{
		// 패배 = 전멸이므로 생존은 0 / 파티 수.
		Survivor->SetText(FText::Format(LOCTEXT("SurvivorFormat", "0 / {0}"),
			FText::AsNumber(mCombatResult.mPartyPortraits.Num())));
	}

	// 파티에 없는 자리는 카드째로 접는다 -- WBP에 구워진 기본 초상(디자이너
	// 견본)이 실제 파티처럼 새어 나오던 문제(0809).
	UImage* PortraitImages[] = { mPartyPortrait0, mPartyPortrait1, mPartyPortrait2 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(PortraitImages); ++Index)
	{
		UTexture2D* Portrait = mCombatResult.mPartyPortraits.IsValidIndex(Index)
			? mCombatResult.mPartyPortraits[Index] : nullptr;
		const bool bHasMember = Portrait != nullptr;
		if (UWidget* CardMount = GetWidgetFromName(FName(*FString::Printf(
			TEXT("DefeatCardFrame_%dMount"), Index))))
		{
			CardMount->SetVisibility(bHasMember
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		}
		UImage* PortraitImage = PortraitImages[Index];
		if (PortraitImage == nullptr)
		{
			continue;
		}
		PortraitImage->SetVisibility(bHasMember
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::Collapsed);
		if (bHasMember == true)
		{
			FVector2D PortraitBoundsPosition(38.0f, 29.0f);
			if (UImage* CardFrame = Cast<UImage>(GetWidgetFromName(FName(*FString::Printf(
				TEXT("DefeatCardFrame_%d"), Index)))))
			{
				if (const UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(CardFrame->Slot))
				{
					PortraitBoundsPosition += FrameSlot->GetPosition();
				}
			}
			SetAspectFitPortrait(PortraitImage, Portrait,
				PortraitBoundsPosition, FVector2D(136.0f, 136.0f));
		}
	}
}

#undef LOCTEXT_NAMESPACE
