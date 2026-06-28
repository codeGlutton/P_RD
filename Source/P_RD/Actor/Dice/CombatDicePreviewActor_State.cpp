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
	mFaceTextureOverrides = NewFaceTextures;

	for (int32 FaceIndex = 0; FaceIndex < mFaceMaterials.Num(); ++FaceIndex)
	{
		UMaterialInstanceDynamic* FaceMaterial = mFaceMaterials[FaceIndex];
		if (FaceMaterial == nullptr)
		{
			continue;
		}

		UTexture* FaceTexture = ResolveFaceTexture(FaceIndex, NewFaceTextures);
		FaceMaterial->SetTextureParameterValue(TEXT("FaceTexture"), FaceTexture);
	}
}

UTexture* ACombatDicePreviewActor::ResolveFaceTexture(int32 FaceIndex, const TArray<TObjectPtr<UTexture>>& NewFaceTextures) const
{
	UTexture* FaceTexture = NewFaceTextures.IsValidIndex(FaceIndex) ? NewFaceTextures[FaceIndex].Get() : nullptr;
	if (FaceTexture != nullptr)
	{
		return FaceTexture;
	}

	if (const FRDCombatDiceFaceTextureSet* DefaultTextureSet = mDefaultFaceTexturesByCount.Find(mCurrentFaceCount))
	{
		const TArray<TObjectPtr<UTexture>>& DefaultTextures = DefaultTextureSet->mTextures;
		if (DefaultTextures.IsValidIndex(FaceIndex) && DefaultTextures[FaceIndex] != nullptr)
		{
			return DefaultTextures[FaceIndex].Get();
		}
	}

	return mDefaultFaceTexture;
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
	if (mDiceMaterial != nullptr)
	{
		// 어두운 숫자 가독성을 위해 몸체는 늘 밝게 유지하고, 상태 색(희귀도/사용 등)은 은은하게만 섞는다.
		const FLinearColor BodyBaseColor = mCurrentFaceCount == 2
			? FLinearColor(0.94f, 0.92f, 0.84f, 1.0f)
			: FLinearColor::White;
		const FLinearColor BodyColor = FMath::Lerp(NewColor, BodyBaseColor, 0.55f);
		mDiceMaterial->SetVectorParameterValue(TEXT("Color"), BodyColor);
	}
	if (mCoinRimMaterial != nullptr)
	{
		const FLinearColor RimColor = mCurrentFaceCount == 2
			? FMath::Lerp(NewColor, FLinearColor(0.48f, 0.50f, 0.52f, 1.0f), 0.72f)
			: FLinearColor::Transparent;
		mCoinRimMaterial->SetVectorParameterValue(TEXT("Color"), RimColor);
	}
}

void ACombatDicePreviewActor::SetBackdropVisible(bool bVisible)
{
	if (mBackdropMesh != nullptr)
	{
		mBackdropMesh->SetVisibility(bVisible);
	}
}
