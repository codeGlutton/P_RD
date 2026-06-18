#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "UObject/ConstructorHelpers.h"

/** @brief 주사위 몸체/림/면 커버에 사용할 동적 머티리얼과 fallback 텍스처를 준비한다. */
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

/** @brief 지원 면 수별 기본 면 커버 텍스처를 이름 규칙으로 로드해 캐시한다. */
void ACombatDicePreviewActor::LoadDefaultFaceTextures()
{
	// [합의필요] 텍스처 네이밍은 T_DiceFace_d{FaceCount}_{FaceIndex:00} 규칙에 의존한다.
	static const int32 SupportedFaceCounts[] = { 2, 4, 6, 8, 12, 20 };
	for (const int32 FaceCount : SupportedFaceCounts)
	{
		FRDCombatDiceFaceTextureSet& TextureSet = mDefaultFaceTexturesByCount.FindOrAdd(FaceCount);
		TArray<TObjectPtr<UTexture>>& Textures = TextureSet.mTextures;
		if (Textures.Num() > 0)
		{
			continue;
		}

		Textures.Reserve(FaceCount);
		for (int32 FaceIndex = 1; FaceIndex <= FaceCount; ++FaceIndex)
		{
			const FString TextureName = FString::Printf(TEXT("T_DiceFace_d%d_%02d"), FaceCount, FaceIndex);
			const FString TexturePath = FString::Printf(
				TEXT("/Game/SVN/OutSideAsset/AICreation/DiceFaceTextures/%s.%s"),
				*TextureName,
				*TextureName);
			Textures.Add(LoadObject<UTexture>(nullptr, *TexturePath));
		}
	}
}
