#include "UI/DiceCapturePreviewUtils.h"

#include "Components/Image.h"
#include "Actor/Dice/CombatDiceCaptureActor.h"

/** @brief HUD 다중 주사위에서도 비용이 과하지 않은 기본 RT 크기를 반환한다. */
int32 RDDiceCapturePreview::GetDefaultRenderTargetSize()
{
	return 512;
}

/** @brief UImage brush 기본 크기를 RenderTarget 크기와 맞춘다. */
FVector2D RDDiceCapturePreview::GetDefaultBrushSize()
{
	const float BrushSize = StaticCast<float>(GetDefaultRenderTargetSize());
	return FVector2D(BrushSize, BrushSize);
}

/** @brief 캡처 액터를 전투 공간과 멀리 떨어진 슬롯별 원거리 위치에 배치한다. */
FVector RDDiceCapturePreview::GetCombatPreviewLocation(int32 GroupIndex, int32 DiceIndex)
{
	// 30000대 좌표는 실제 전투 타일/카메라와 겹치지 않게 하는 격리 영역이다.
	// GroupIndex는 패널 그룹, DiceIndex는 같은 그룹 내 슬롯 간 간격이다.
	return FVector(
		0.0f,
		30000.0f + StaticCast<float>(GroupIndex) * 3500.0f + StaticCast<float>(DiceIndex) * 420.0f,
		30000.0f);
}

/** @brief 캡처 화면 안에서 숫자와 림이 적당히 차는 주사위 스케일 보정값. */
float RDDiceCapturePreview::GetCombatPreviewDiceScale()
{
	return 1.12f;
}

/** @brief CaptureActor의 머티리얼을 UImage brush에 물리고 색 보정을 중립화한다. */
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

/** @brief 캡처 액터를 항상 스폰하고 호출자 outer로 RenderTarget 수명을 묶는다. */
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

/** @brief UI 패널 해제 시 캡처 액터를 Destroy하고 배열 참조를 비운다. */
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
