#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

void ACombatDicePreviewActor::SetDiceRotation(const FRotator& NewRotation)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetRelativeRotation(NewRotation);
	}
}

void ACombatDicePreviewActor::SetFaceValues(const TArray<int32>& NewFaceValues)
{
	for (int32 FaceIndex = 0; FaceIndex < NewFaceValues.Num(); ++FaceIndex)
	{
		SetFaceText(FaceIndex + 1, FText::AsNumber(NewFaceValues[FaceIndex]));
	}
}

void ACombatDicePreviewActor::SetFaceTextures(const TArray<TObjectPtr<UTexture>>& NewFaceTextures)
{
	// Material-only dice: face textures are intentionally ignored.
}

void ACombatDicePreviewActor::SetFaceData(const TArray<int32>& NewFaceValues, const TArray<TObjectPtr<UTexture>>& NewFaceTextures)
{
	SetFaceValues(NewFaceValues);
	SetFaceTextures(NewFaceTextures);
}

void ACombatDicePreviewActor::SettleToFace(int32 FaceValue)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetRelativeRotation(GetSettledFaceRotation(FaceValue));
	}
}

void ACombatDicePreviewActor::SetDiceWorldTransform(const FTransform& NewTransform)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetWorldTransform(NewTransform);
	}
}

void ACombatDicePreviewActor::SetDiceVisibleInSceneCaptureOnly(bool bSceneCaptureOnly)
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents(PrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (PrimitiveComponent != nullptr)
		{
			PrimitiveComponent->SetVisibleInSceneCaptureOnly(bSceneCaptureOnly);
		}
	}
}

void ACombatDicePreviewActor::SetPreviewLightingEnabled(bool bEnabled)
{
	if (mFrontFillLight != nullptr)
	{
		mFrontFillLight->SetVisibility(bEnabled);
		mFrontFillLight->SetHiddenInGame(!bEnabled);
		mFrontFillLight->SetIntensity(bEnabled ? 4200.0f : 0.0f);
	}
	if (mSideFillLight != nullptr)
	{
		mSideFillLight->SetVisibility(bEnabled);
		mSideFillLight->SetHiddenInGame(!bEnabled);
		mSideFillLight->SetIntensity(bEnabled ? 2600.0f : 0.0f);
	}
	if (mTopFillLight != nullptr)
	{
		mTopFillLight->SetVisibility(bEnabled);
		mTopFillLight->SetHiddenInGame(!bEnabled);
		mTopFillLight->SetIntensity(bEnabled ? 1900.0f : 0.0f);
	}
}

void ACombatDicePreviewActor::SetDiceColor(const FLinearColor& NewColor)
{
	mDiceTintColor = NewColor;
	if (mDiceMaterial != nullptr)
	{
		ApplyDiceBodyMaterialPreset();
	}
	if (mCoinRimMaterial != nullptr)
	{
		const FLinearColor RimColor = mCurrentFaceCount == 2
			? FMath::Lerp(NewColor, FLinearColor(0.48f, 0.50f, 0.52f, 1.0f), 0.72f)
			: FLinearColor::Transparent;
		mCoinRimMaterial->SetVectorParameterValue(TEXT("Color"), RimColor);
	}
}

void ACombatDicePreviewActor::SetDiceMaterialVariant(int32 VariantIndex)
{
	mDiceMaterialVariantIndex = VariantIndex;
	ApplyDiceBodyMaterialPreset();
}

void ACombatDicePreviewActor::SetBackdropVisible(bool bVisible)
{
	if (mBackdropMesh != nullptr)
	{
		mBackdropMesh->SetVisibility(bVisible);
	}
}
