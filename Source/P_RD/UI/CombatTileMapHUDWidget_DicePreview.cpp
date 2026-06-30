#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"
#include "Actor/Dice/CombatDiceRollCaptureActor.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/DiceCapturePreviewUtils.h"

using namespace RDCombatHUD;

// 두 헬퍼는 Private 헤더에 선언만 두고 여기서 한 번만 정의한다(DiceRoll.cpp와 중복정의→유니티 충돌 방지).
namespace RDCombatHUD
{
	/** @brief d2/d4는 비대칭 실루엣이라 idle 회전보다 확정 면 정면 자세가 읽기 쉽다. */
	bool ShouldUseFaceOnReadyPose(const FDiceViewData* DiceView)
	{
		return DiceView != nullptr && (DiceView->mFaceCount == 2 || DiceView->mFaceCount == 4);
	}

	/** @brief 주사위 종류별 대기 자세를 통일해 입장 연출과 보유 주사위 카드의 첫 프레임을 맞춘다. */
	FRotator GetDiceReadyRotation(const FDiceViewData* DiceView, int32 DiceIndex, const ACombatDiceCaptureActor* DiceActor)
	{
		if (ShouldUseFaceOnReadyPose(DiceView) && DiceActor != nullptr)
		{
			return DiceActor->GetSettledFaceRotation(1);
		}
		return GetReadableDiceIdleRotation(DiceIndex);
	}
}

/** @brief 캡처 액터가 가진 투명 RenderTarget을 UMG Image에 연결하는 얇은 경계 함수. */
void UCombatTileMapHUDWidget::ApplyDiceCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize) const
{
	RDDiceCapturePreview::ApplyCaptureBrush(DiceImage, DiceActor, BrushSize);
}

/** @brief 전투 HUD용 3D 주사위를 실제 전투 월드와 분리된 UI 전용 좌표에 생성한다. */
ACombatDiceCaptureActor* UCombatTileMapHUDWidget::SpawnDiceCaptureActor(int32 GroupIndex, int32 DiceIndex, int32 RenderTargetSize)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	return RDDiceCapturePreview::SpawnCaptureActor(World, this, RDDiceCapturePreview::GetCombatPreviewLocation(GroupIndex, DiceIndex), RenderTargetSize);
}

/** @brief UMG 위젯 수명에 맞춰 SceneCapture 액터를 정리해 이전 RenderTarget 참조가 남지 않게 한다. */
void UCombatTileMapHUDWidget::DestroyDiceCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors) const
{
	RDDiceCapturePreview::DestroyCaptureActors(DiceActors);
}

void UCombatTileMapHUDWidget::EnsureDiceRollPhysicsActor()
{
	if (mDiceRollPhysicsActor != nullptr && IsValid(mDiceRollPhysicsActor))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	mDiceRollPhysicsActor = World->SpawnActor<ACombatDiceRollCaptureActor>(
		ACombatDiceRollCaptureActor::StaticClass(),
		RDDiceCapturePreview::GetCombatPreviewLocation(2, 0),
		FRotator::ZeroRotator,
		SpawnParameters
	);
	if (mDiceRollPhysicsActor == nullptr)
	{
		return;
	}

	mDiceRollPhysicsActor->InitializeCapture(this, 1536, 704);
	if (mDiceRollPhysicsImage != nullptr && mDiceRollPhysicsActor->GetCaptureMaterial() != nullptr)
	{
		FSlateBrush DiceBrush = mDiceRollPhysicsImage->GetBrush();
		DiceBrush.SetResourceObject(mDiceRollPhysicsActor->GetCaptureMaterial());
		DiceBrush.ImageSize = FVector2D(1536.0f, 704.0f);
		mDiceRollPhysicsImage->SetBrush(DiceBrush);
		mDiceRollPhysicsImage->SetColorAndOpacity(FLinearColor::White);
	}
}

void UCombatTileMapHUDWidget::DestroyDiceRollPhysicsActor()
{
	if (IsValid(mDiceRollPhysicsActor))
	{
		mDiceRollPhysicsActor->Destroy();
	}
	mDiceRollPhysicsActor = nullptr;
}

/** @brief 입장 굴림용 Image/캡처 액터를 현재 주사위 수에 맞춰 재구성한다. */
void UCombatTileMapHUDWidget::EnsureDicePreviewActors()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	for (UImage* DiceImage : mDiceRollImages)
	{
		if (DiceImage != nullptr)
		{
			DiceImage->RemoveFromParent();
		}
	}
	mDiceRollImages.Reset();
	for (UImage* ShadowImage : mDiceRollShadowImages)
	{
		if (ShadowImage != nullptr)
		{
			ShadowImage->RemoveFromParent();
		}
	}
	mDiceRollShadowImages.Reset();
	DestroyDiceCaptureActors(mDicePreviewActors);
	mIntroDiceSettleStartRotations.Reset();

	const int32 DiceCount = FMath::Min(mDiceUIs.Num(), MaxCombatDiceCardCount);
	if (DiceCount <= 0)
	{
		ApplyRuntimeWidgetLayout();
		return;
	}

	for (int32 DiceIndex = 0; DiceIndex < DiceCount; ++DiceIndex)
	{
		UImage* ShadowImage = nullptr;
		if (mDiceRollShadowTexture != nullptr)
		{
			ShadowImage = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				FName(*FString::Printf(TEXT("DiceRollShadowImage_%d"), DiceIndex))
			);
			if (ShadowImage != nullptr)
			{
				ShadowImage->SetBrushFromTexture(mDiceRollShadowTexture, true);
				ShadowImage->SetColorAndOpacity(FLinearColor(0.0f, 0.18f, 0.28f, 0.68f));
				ShadowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
				RootCanvas->AddChildToCanvas(ShadowImage);
			}
		}

		UImage* DiceImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("DiceRollImage_%d"), DiceIndex))
		);
		if (DiceImage == nullptr)
		{
			if (ShadowImage != nullptr)
			{
				ShadowImage->RemoveFromParent();
			}
			continue;
		}

		DiceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		RootCanvas->AddChildToCanvas(DiceImage);

		ACombatDiceCaptureActor* DicePreviewActor = SpawnDiceCaptureActor(0, DiceIndex, RDDiceCapturePreview::GetDefaultRenderTargetSize());
		const FDiceViewData* DiceView = mDiceUIs.IsValidIndex(DiceIndex) ? &mDiceUIs[DiceIndex] : nullptr;
		FRotator StartRotation = GetReadableDiceIdleRotation(DiceIndex);
		if (DicePreviewActor != nullptr)
		{
			DicePreviewActor->SetDiceType(DiceView != nullptr ? DiceView->mFaceCount : 6);
			if (DiceView != nullptr)
			{
				DicePreviewActor->SetFaceData(DiceView->mFaceValues, DiceView->mFaceTextures);
			}
			DicePreviewActor->SetActorScale3D(FVector(RDDiceCapturePreview::GetCombatPreviewDiceScale()));
			StartRotation = GetDiceReadyRotation(DiceView, DiceIndex, DicePreviewActor);
			DicePreviewActor->SetDiceRotation(StartRotation);
			DicePreviewActor->SetBackdropVisible(false);
			DicePreviewActor->CaptureDice();
			ApplyDiceCaptureBrush(DiceImage, DicePreviewActor, RDDiceCapturePreview::GetDefaultBrushSize());
		}

		mDiceRollShadowImages.Add(ShadowImage);
		mDiceRollImages.Add(DiceImage);
		mDicePreviewActors.Add(DicePreviewActor);
		// 굴림 후반부 보간의 시작 회전값. 액터가 재생성되어도 결과 정착이 튀지 않도록 배열에 별도로 보존한다.
		mIntroDiceSettleStartRotations.Add(StartRotation);
	}

	ApplyRuntimeWidgetLayout();
}

/** @brief 캡처 액터가 GC/월드 전환으로 사라졌을 때 같은 주사위 데이터로 복구한다. */
void UCombatTileMapHUDWidget::RefreshDicePreviewActors()
{
	for (int32 DiceIndex = 0; DiceIndex < mDiceRollImages.Num(); ++DiceIndex)
	{
		UImage* DiceImage = mDiceRollImages[DiceIndex];
		if (DiceImage == nullptr)
		{
			continue;
		}

		if (mDicePreviewActors.IsValidIndex(DiceIndex) == false)
		{
			mDicePreviewActors.SetNum(DiceIndex + 1);
		}

		if (IsValid(mDicePreviewActors[DiceIndex]) == false)
		{
			mDicePreviewActors[DiceIndex] = SpawnDiceCaptureActor(0, DiceIndex, RDDiceCapturePreview::GetDefaultRenderTargetSize());
			if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
			{
				const FDiceViewData* DiceView = mDiceUIs.IsValidIndex(DiceIndex) ? &mDiceUIs[DiceIndex] : nullptr;
				const FLinearColor DiceColor = mDiceUIs.IsValidIndex(DiceIndex)
					? RDUIDice::GetDiceRarityColor(mDiceUIs[DiceIndex])
					: RDUIDice::GetDiceRarityColor(ERarityType::Common);

				DicePreviewActor->SetDiceType(DiceView != nullptr ? DiceView->mFaceCount : 6);
				if (DiceView != nullptr)
				{
					DicePreviewActor->SetFaceData(DiceView->mFaceValues, DiceView->mFaceTextures);
				}
				const FRotator StartRotation = mIntroDiceSettleStartRotations.IsValidIndex(DiceIndex)
					? mIntroDiceSettleStartRotations[DiceIndex]
					: GetDiceReadyRotation(DiceView, DiceIndex, DicePreviewActor);
				DicePreviewActor->SetActorScale3D(FVector(RDDiceCapturePreview::GetCombatPreviewDiceScale()));
				DicePreviewActor->SetDiceRotation(StartRotation);
				DicePreviewActor->SetDiceColor(DiceColor);
				DicePreviewActor->SetBackdropVisible(false);
				DicePreviewActor->CaptureDice();
			}
		}

		if (ACombatDiceCaptureActor* DicePreviewActor = mDicePreviewActors[DiceIndex])
		{
			ApplyDiceCaptureBrush(DiceImage, DicePreviewActor, RDDiceCapturePreview::GetDefaultBrushSize());
		}
	}
}
