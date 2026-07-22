#include "UI/CombatTileMapHUDWidget.h"

#include "Engine/AssetManager.h"   // 턴 전환 프레임 비동기 프리로드(첫 배너 동기 로드 히치 제거)
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "TimerManager.h"   // 배너 강제 종료 보험 타이머(Tick 정지 시 배리어 soft-lock 방지)
#include "UI/Combat/CombatUIModel.h"
#include "Singleton/WorldSubsystem/PresentationBarrier.h"   // mRoundChangeBarrier.Reset() 시 소멸자(=라운드 첫 턴 진행) 실행

namespace
{
	constexpr int32 TurnChangeFrameCount = 33;
	constexpr float TurnChangeFramesPerSecond = 16.0f;
	// 원본 33장을 다 쓰지 않고 2장에 1장만 쓴다 — 로드 개수·상주 메모리 절반, 같은 fps라 재생은 2배 빨라진다(≈1.06s).
	// (배너는 라운드 배리어를 잡아 게임 진행을 막으므로 짧을수록 턴 템포에 유리.)
	constexpr int32 TurnChangeFrameStep = 2;
	constexpr int32 TurnChangeUsedFrameCount = (TurnChangeFrameCount + TurnChangeFrameStep - 1) / TurnChangeFrameStep;
	const FVector2D TurnChangeFrameNativeSize(832.0f, 448.0f);
	const TCHAR* const TurnChangeFrameAssetDirectory = TEXT("/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/TurnChange/FramesBiRefNet");

	int32 GetClampedFrameIndex(float ElapsedSeconds)
	{
		const int32 FrameIndex = FMath::FloorToInt(ElapsedSeconds * TurnChangeFramesPerSecond);
		return FMath::Clamp(FrameIndex, 0, TurnChangeUsedFrameCount - 1);
	}
}

bool UCombatTileMapHUDWidget::PlayTurnChangeIntro()
{
	EnsureRuntimeWidgets();

	if (EnsureTurnChangeFrameTextures() == false)
	{
		return false;
	}

	if (mTurnChangeIntroPlaying == true)
	{
		return true;
	}

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

	// 보험 타이머: 정상 종료는 NativeTick 경과시간 판정뿐이라, 배너 중 Tick이 멈추면
	// 라운드 배리어가 영영 안 풀린다(soft-lock). 재생시간+1초가 지나면 무조건 종료시킨다.
	// (타이머는 위젯 Tick과 무관하게 월드가 돌리고, 정상 종료가 먼저 오면 아래 Finish에서 지운다.)
	if (UWorld* World = GetWorld())
	{
		const float SafetyDelay = StaticCast<float>(TurnChangeFrameCount) / TurnChangeFramesPerSecond + 1.0f;
		World->GetTimerManager().SetTimer(
			mTurnChangeSafetyTimerHandle,
			FTimerDelegate::CreateUObject(this, &UCombatTileMapHUDWidget::FinishTurnChangeIntro),
			SafetyDelay,
			false);
	}
	return true;
}

void UCombatTileMapHUDWidget::FinishTurnChangeIntro()
{
	if (mTurnChangeIntroPlaying == false)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(mTurnChangeSafetyTimerHandle);
	}

	mTurnChangeIntroPlaying = false;
	mTurnChangeIntroElapsed = 0.0f;
	mTurnChangeCurrentFrameIndex = INDEX_NONE;
	SetTurnChangeIntroVisibility(false);

	// 라운드 시작 배너로 잡아둔 배리어가 있으면 여기서 놓는다 → 마지막 참조 소멸 시 프레임워크가 그 라운드의 첫 턴을 진행한다.
	// (라운드가 아닌 경로로 배너가 떴다면 mRoundChangeBarrier는 비어 있어 Reset()은 무해한 no-op.)
	// HUD 상태 정리 뒤 마지막에 놓아 재진입을 피한다.
	mRoundChangeBarrier.Reset();
}

bool UCombatTileMapHUDWidget::EnsureTurnChangeFrameTextures()
{
	if (mTurnChangeFrameTextures.Num() == TurnChangeUsedFrameCount)
	{
		return true;
	}

	// PreloadTurnChangeFrameTextures가 전투 진입 시 비동기로 미리 올려두므로,
	// 여기 LoadObject는 보통 이미 로드된 에셋을 찾는 빠른 경로다(프리로드 전 첫 배너 폴백만 동기 로드).
	mTurnChangeFrameTextures.Reset();
	mTurnChangeFrameTextures.Reserve(TurnChangeUsedFrameCount);
	for (int32 UsedIndex = 0; UsedIndex < TurnChangeUsedFrameCount; ++UsedIndex)
	{
		UTexture2D* FrameTexture = LoadTurnChangeFrameTexture(ResolveTurnChangeFrameAssetPath(UsedIndex * TurnChangeFrameStep));
		if (FrameTexture == nullptr)
		{
			mTurnChangeFrameTextures.Reset();
			return false;
		}
		mTurnChangeFrameTextures.Add(FrameTexture);
	}

	return true;
}

void UCombatTileMapHUDWidget::PreloadTurnChangeFrameTextures()
{
	if (mTurnChangeFramePreloadHandle.IsValid() || mTurnChangeFrameTextures.Num() == TurnChangeUsedFrameCount)
	{
		return;
	}

	// 전투 진입(HUD 초기화) 시점에 비동기로 올려둔다 — 기존엔 첫 라운드 배너가 뜨는 순간
	// 게임 스레드에서 프레임 텍스처를 동기 로드 루프로 읽어 확정적 히치가 있었다.
	TArray<FSoftObjectPath> FramePaths;
	FramePaths.Reserve(TurnChangeUsedFrameCount);
	for (int32 UsedIndex = 0; UsedIndex < TurnChangeUsedFrameCount; ++UsedIndex)
	{
		FramePaths.Add(FSoftObjectPath(ResolveTurnChangeFrameAssetPath(UsedIndex * TurnChangeFrameStep)));
	}
	mTurnChangeFramePreloadHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(FramePaths));
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

	const float TotalDuration = StaticCast<float>(TurnChangeUsedFrameCount) / TurnChangeFramesPerSecond;
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
