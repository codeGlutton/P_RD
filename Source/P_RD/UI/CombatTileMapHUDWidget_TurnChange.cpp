#include "UI/CombatTileMapHUDWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"

namespace
{
	constexpr int32 TurnChangeFrameCount = 33;
	constexpr float TurnChangeFramesPerSecond = 16.0f;
	const FVector2D TurnChangeFrameNativeSize(832.0f, 448.0f);
	const TCHAR* const TurnChangeFrameAssetDirectory = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/TurnChange/FramesBiRefNet");

	int32 GetClampedFrameIndex(float ElapsedSeconds)
	{
		const int32 FrameIndex = FMath::FloorToInt(ElapsedSeconds * TurnChangeFramesPerSecond);
		return FMath::Clamp(FrameIndex, 0, TurnChangeFrameCount - 1);
	}
}

bool UCombatTileMapHUDWidget::PlayTurnChangeIntro(bool bOpenDiceAfterIntro)
{
	EnsureRuntimeWidgets();

	if (EnsureTurnChangeFrameTextures() == false)
	{
		if (bOpenDiceAfterIntro == true
			&& mIntroDiceRollActive == false
			&& mIntroDiceRollReady == false
			&& mIntroDiceResultWaitingForDismiss == false)
		{
			PrepareIntroDiceRoll();
		}
		return false;
	}

	if (mTurnChangeIntroPlaying == true)
	{
		mPendingDiceRollAfterTurnIntro = mPendingDiceRollAfterTurnIntro || bOpenDiceAfterIntro;
		return true;
	}

	mPendingDiceRollAfterTurnIntro = bOpenDiceAfterIntro;
	mPendingInitialTurnChangeIntro = false;
	mTurnChangeIntroPlaying = true;
	mTurnChangeIntroElapsed = 0.0f;
	mTurnChangeCurrentFrameIndex = INDEX_NONE;

	if (mTurnRoundBannerText != nullptr)
	{
		mTurnRoundBannerText->SetText(FText::GetEmpty());
		mTurnRoundBannerText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (mTurnChangeTurnText != nullptr)
	{
		mTurnChangeTurnText->SetText(FText::AsNumber(GetTurnChangeDisplayNumber()));
	}

	ApplyTurnChangeFrame(0);
	SetTurnChangeIntroVisibility(true);
	return true;
}

void UCombatTileMapHUDWidget::FinishTurnChangeIntro()
{
	if (mTurnChangeIntroPlaying == false)
	{
		return;
	}

	mTurnChangeIntroPlaying = false;
	mTurnChangeIntroElapsed = 0.0f;
	mTurnChangeCurrentFrameIndex = INDEX_NONE;
	SetTurnChangeIntroVisibility(false);

	const bool bOpenDiceAfterIntro = mPendingDiceRollAfterTurnIntro;
	mPendingDiceRollAfterTurnIntro = false;
	if (bOpenDiceAfterIntro == true
		&& mIntroDiceRollActive == false
		&& mIntroDiceRollReady == false
		&& mIntroDiceResultWaitingForDismiss == false)
	{
		PrepareIntroDiceRoll();
	}
}

bool UCombatTileMapHUDWidget::EnsureTurnChangeFrameTextures()
{
	if (mTurnChangeFrameTextures.Num() == TurnChangeFrameCount)
	{
		return true;
	}

	mTurnChangeFrameTextures.Reset();
	mTurnChangeFrameTextures.Reserve(TurnChangeFrameCount);
	for (int32 FrameIndex = 0; FrameIndex < TurnChangeFrameCount; ++FrameIndex)
	{
		UTexture2D* FrameTexture = LoadTurnChangeFrameTexture(ResolveTurnChangeFrameAssetPath(FrameIndex));
		if (FrameTexture == nullptr)
		{
			mTurnChangeFrameTextures.Reset();
			return false;
		}
		mTurnChangeFrameTextures.Add(FrameTexture);
	}

	return true;
}

void UCombatTileMapHUDWidget::SetTurnChangeIntroVisibility(bool bVisible) const
{
	// 턴 전환 텍스처 에셋은 알파를 가지므로 영상 자체 뒤에는 딤 배경을 깔지 않는다.
	if (mTurnChangeBackdropPanel != nullptr)
	{
		mTurnChangeBackdropPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (mTurnChangeVideoImage != nullptr)
	{
		mTurnChangeVideoImage->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mTurnChangeTurnTextPanel != nullptr)
	{
		mTurnChangeTurnTextPanel->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (mTurnChangeInputBlocker != nullptr)
	{
		mTurnChangeInputBlocker->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

int32 UCombatTileMapHUDWidget::GetTurnChangeDisplayNumber() const
{
	const int32 Round = mCombatUIModel != nullptr ? mCombatUIModel->GetTurnUI().mRound : 0;
	return Round > 0 ? Round : FMath::Max(1, mLastShownTurnRound + 1);
}

FString UCombatTileMapHUDWidget::ResolveTurnChangeFrameAssetPath(int32 FrameIndex) const
{
	const FString AssetName = FString::Printf(TEXT("T_TurnChange_%03d"), FrameIndex + 1);
	return FString::Printf(
		TEXT("%s/%s.%s"),
		TurnChangeFrameAssetDirectory,
		*AssetName,
		*AssetName);
}

UTexture2D* UCombatTileMapHUDWidget::LoadTurnChangeFrameTexture(const FString& FrameAssetPath) const
{
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *FrameAssetPath);
	if (Texture == nullptr)
	{
		UE_LOG(LogRD, Warning, TEXT("Combat turn change frame asset missing: %s"), *FrameAssetPath);
		return nullptr;
	}

	return Texture;
}

void UCombatTileMapHUDWidget::UpdateTurnChangeIntro(float InDeltaTime)
{
	mTurnChangeIntroElapsed += FMath::Max(0.0f, InDeltaTime);

	const float TotalDuration = StaticCast<float>(TurnChangeFrameCount) / TurnChangeFramesPerSecond;
	if (mTurnChangeIntroElapsed >= TotalDuration)
	{
		FinishTurnChangeIntro();
		return;
	}

	ApplyTurnChangeFrame(GetClampedFrameIndex(mTurnChangeIntroElapsed));
}

void UCombatTileMapHUDWidget::ApplyTurnChangeFrame(int32 FrameIndex)
{
	if (mTurnChangeVideoImage == nullptr || mTurnChangeFrameTextures.IsValidIndex(FrameIndex) == false)
	{
		return;
	}
	if (mTurnChangeCurrentFrameIndex == FrameIndex)
	{
		return;
	}

	UTexture2D* FrameTexture = mTurnChangeFrameTextures[FrameIndex];
	if (FrameTexture == nullptr)
	{
		return;
	}

	mTurnChangeCurrentFrameIndex = FrameIndex;
	mTurnChangeFrameBrush.DrawAs = ESlateBrushDrawType::Image;
	mTurnChangeFrameBrush.ImageSize = TurnChangeFrameNativeSize;
	mTurnChangeFrameBrush.TintColor = FSlateColor(FLinearColor::White);
	mTurnChangeFrameBrush.SetResourceObject(FrameTexture);
	mTurnChangeVideoImage->SetColorAndOpacity(FLinearColor::White);
	mTurnChangeVideoImage->SetRenderOpacity(1.0f);
	mTurnChangeVideoImage->SetBrush(mTurnChangeFrameBrush);
}
