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

	// 캡처 해상도는 표시 크기(1536x704)의 절반 — 굴림 내내 반복되는 풀 씬 HDR 캡처의
	// 픽셀 수(≈4.3MB RT)를 1/4로 줄인다. 표시 크기는 브러시 ImageSize가 그대로 유지한다.
	mDiceRollPhysicsActor->InitializeCapture(this, 768, 352);
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
