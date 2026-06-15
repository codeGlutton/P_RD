#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
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
	for (int32 FaceIndex = 0; FaceIndex < NewFaceValues.Num() && FaceIndex < mFaceTexts.Num(); ++FaceIndex)
	{
		SetFaceText(FaceIndex + 1, FText::AsNumber(NewFaceValues[FaceIndex]));
	}
}

void ACombatDicePreviewActor::SettleToFace(int32 FaceValue)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetRelativeRotation(GetSettledFaceRotation(FaceValue));
	}
}

void ACombatDicePreviewActor::SetDiceColor(const FLinearColor& NewColor)
{
	if (mDiceMaterial != nullptr)
	{
		mDiceMaterial->SetVectorParameterValue(TEXT("Color"), NewColor);
		mDiceMaterial->SetVectorParameterValue(TEXT("TintColor"), NewColor);
	}
}

void ACombatDicePreviewActor::SetBackdropVisible(bool bVisible)
{
	if (mBackdropMesh != nullptr)
	{
		mBackdropMesh->SetVisibility(bVisible);
	}
}
