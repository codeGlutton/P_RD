#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "UObject/ConstructorHelpers.h"

void ACombatDicePreviewActor::InitializeMaterials()
{
	// 주사위 몸체: 자발광 보강 머티리얼(어두운 면도 검게 죽지 않게). 'Color'를 SetDiceColor로 갱신.
	// 메시 적용은 ApplyDiceMesh에서 mDiceMesh->SetMaterial(0, ...)로.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DiceBodyMaterialFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/M_DiceBody.M_DiceBody"));
	if (DiceBodyMaterialFinder.Succeeded())
	{
		mDiceMaterial = UMaterialInstanceDynamic::Create(DiceBodyMaterialFinder.Object, this);
		if (mDiceMaterial != nullptr)
		{
			mDiceMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.92f, 0.90f, 0.84f, 1.0f));
		}
	}

	// 배경판.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ColorMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ColorMaterialFinder.Succeeded() && mBackdropMesh != nullptr)
	{
		mBackdropMaterial = mBackdropMesh->CreateDynamicMaterialInstance(0, ColorMaterialFinder.Object);
		if (mBackdropMaterial != nullptr)
		{
			mBackdropMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.46f, 0.64f, 0.78f, 1.0f));
		}
		if (mCoinRimMesh != nullptr)
		{
			mCoinRimMaterial = UMaterialInstanceDynamic::Create(ColorMaterialFinder.Object, this);
			if (mCoinRimMaterial != nullptr)
			{
				mCoinRimMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.54f, 0.56f, 0.58f, 1.0f));
			}
		}
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DiceFaceMaterialFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/M_DiceFace.M_DiceFace"));
	if (DiceFaceMaterialFinder.Succeeded())
	{
		mFaceMaterialTemplate = DiceFaceMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UTexture> DefaultFaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_Base.T_DiceFace_Base"));
	if (DefaultFaceTextureFinder.Succeeded())
	{
		mDefaultFaceTexture = DefaultFaceTextureFinder.Object;
	}
}

void ACombatDicePreviewActor::LoadDefaultFaceTextures()
{
	struct FDefaultFaceTextureEntry
	{
		int32 mFaceCount;
		UTexture* mTexture;
	};

	static ConstructorHelpers::FObjectFinder<UTexture> D2FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D2.T_DiceFace_D2"));
	static ConstructorHelpers::FObjectFinder<UTexture> D4FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D4.T_DiceFace_D4"));
	static ConstructorHelpers::FObjectFinder<UTexture> D6FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D6.T_DiceFace_D6"));
	static ConstructorHelpers::FObjectFinder<UTexture> D8FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D8.T_DiceFace_D8"));
	static ConstructorHelpers::FObjectFinder<UTexture> D12FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D12.T_DiceFace_D12"));
	static ConstructorHelpers::FObjectFinder<UTexture> D20FaceTextureFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/T_DiceFace_D20.T_DiceFace_D20"));

	const FDefaultFaceTextureEntry Entries[] = {
		{ 2, D2FaceTextureFinder.Succeeded() ? D2FaceTextureFinder.Object : mDefaultFaceTexture },
		{ 4, D4FaceTextureFinder.Succeeded() ? D4FaceTextureFinder.Object : mDefaultFaceTexture },
		{ 6, D6FaceTextureFinder.Succeeded() ? D6FaceTextureFinder.Object : mDefaultFaceTexture },
		{ 8, D8FaceTextureFinder.Succeeded() ? D8FaceTextureFinder.Object : mDefaultFaceTexture },
		{ 12, D12FaceTextureFinder.Succeeded() ? D12FaceTextureFinder.Object : mDefaultFaceTexture },
		{ 20, D20FaceTextureFinder.Succeeded() ? D20FaceTextureFinder.Object : mDefaultFaceTexture },
	};

	for (const FDefaultFaceTextureEntry& Entry : Entries)
	{
		FRDCombatDiceFaceTextureSet& TextureSet = mDefaultFaceTexturesByCount.FindOrAdd(Entry.mFaceCount);
		TArray<TObjectPtr<UTexture>>& Textures = TextureSet.mTextures;
		if (Textures.Num() > 0)
		{
			continue;
		}

		Textures.Reserve(Entry.mFaceCount);
		for (int32 FaceIndex = 0; FaceIndex < Entry.mFaceCount; ++FaceIndex)
		{
			Textures.Add(Entry.mTexture);
		}
	}
}
