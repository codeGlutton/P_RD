#include "UI/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

void ACombatDicePreviewActor::InitializeMaterials()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DiceFaceMaterialFinder(TEXT("/Game/BP/UI/Dice/M_DiceFace.M_DiceFace"));
	if (DiceFaceMaterialFinder.Succeeded())
	{
		mDiceMaterial = mDiceMesh->CreateDynamicMaterialInstance(0, DiceFaceMaterialFinder.Object);
		if (mDiceMaterial != nullptr)
		{
			mDiceMaterial->SetVectorParameterValue(TEXT("TintColor"), FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> CubeMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (CubeMaterialFinder.Succeeded())
	{
		mBackdropMaterial = mBackdropMesh->CreateDynamicMaterialInstance(0, CubeMaterialFinder.Object);
		if (mBackdropMaterial != nullptr)
		{
			mBackdropMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.46f, 0.64f, 0.78f, 1.0f));
		}

		if (mDiceMaterial == nullptr)
		{
			mDiceMaterial = mDiceMesh->CreateDynamicMaterialInstance(0, CubeMaterialFinder.Object);
			if (mDiceMaterial != nullptr)
			{
				mDiceMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.88f, 0.98f, 0.95f, 1.0f));
			}
		}

		mEdgeMaterial = UMaterialInstanceDynamic::Create(CubeMaterialFinder.Object, this);
		if (mEdgeMaterial != nullptr)
		{
			mEdgeMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.58f, 0.75f, 0.72f, 1.0f));
		}
	}
}
