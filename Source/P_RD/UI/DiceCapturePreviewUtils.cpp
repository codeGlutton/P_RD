#include "UI/DiceCapturePreviewUtils.h"

#include "Components/Image.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"

int32 RDDiceCapturePreview::GetDefaultRenderTargetSize()
{
	return 512;
}

FVector2D RDDiceCapturePreview::GetDefaultBrushSize()
{
	const float BrushSize = StaticCast<float>(GetDefaultRenderTargetSize());
	return FVector2D(BrushSize, BrushSize);
}

FVector RDDiceCapturePreview::GetCombatPreviewLocation(int32 GroupIndex, int32 DiceIndex)
{
	return FVector(
		0.0f,
		30000.0f + StaticCast<float>(GroupIndex) * 3500.0f + StaticCast<float>(DiceIndex) * 420.0f,
		30000.0f);
}

float RDDiceCapturePreview::GetCombatPreviewDiceScale()
{
	return 1.14f;
}

void RDDiceCapturePreview::ApplyCaptureBrush(UImage* DiceImage, ACombatDiceCaptureActor* DiceActor, FVector2D BrushSize)
{
	if (DiceImage == nullptr || DiceActor == nullptr || DiceActor->GetCaptureMaterial() == nullptr)
	{
		return;
	}

	FSlateBrush DiceBrush = DiceImage->GetBrush();
	DiceBrush.SetResourceObject(DiceActor->GetCaptureMaterial());
	DiceBrush.ImageSize = BrushSize;
	DiceImage->SetBrush(DiceBrush);
	DiceImage->SetColorAndOpacity(FLinearColor::White);
}

ACombatDiceCaptureActor* RDDiceCapturePreview::SpawnCaptureActor(UWorld* World, UObject* RenderTargetOuter, const FVector& PreviewLocation, int32 RenderTargetSize)
{
	if (World == nullptr || RenderTargetOuter == nullptr)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACombatDiceCaptureActor* DiceActor = World->SpawnActor<ACombatDiceCaptureActor>(
		ACombatDiceCaptureActor::StaticClass(),
		PreviewLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (DiceActor != nullptr)
	{
		DiceActor->InitializeCapture(RenderTargetOuter, RenderTargetSize);
		DiceActor->SetBackdropVisible(false);
	}
	return DiceActor;
}

void RDDiceCapturePreview::DestroyCaptureActors(TArray<TObjectPtr<ACombatDiceCaptureActor>>& DiceActors)
{
	for (ACombatDiceCaptureActor* DiceActor : DiceActors)
	{
		if (IsValid(DiceActor))
		{
			DiceActor->Destroy();
		}
	}
	DiceActors.Reset();
}
