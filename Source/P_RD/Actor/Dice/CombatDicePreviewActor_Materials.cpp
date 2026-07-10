#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	struct FDiceBodyMaterialPreset
	{
		FLinearColor mBaseColor;
		float mMetallic = 0.0f;
		float mSpecular = 0.52f;
		float mRoughness = 0.36f;
		float mEmissiveStrength = 0.06f;
	};

	const FDiceBodyMaterialPreset& GetDiceBodyMaterialPreset(int32 VariantIndex)
	{
		static const FDiceBodyMaterialPreset Presets[] = {
			{ FLinearColor(0.91f, 0.86f, 0.73f, 1.0f), 0.00f, 0.52f, 0.38f, 0.06f }, // warm satin
			{ FLinearColor(0.80f, 0.89f, 0.96f, 1.0f), 0.00f, 0.68f, 0.22f, 0.04f }, // cool glossy
			{ FLinearColor(0.92f, 0.78f, 0.46f, 1.0f), 0.35f, 0.62f, 0.30f, 0.04f }, // soft brass
			{ FLinearColor(0.74f, 0.88f, 0.79f, 1.0f), 0.00f, 0.45f, 0.58f, 0.05f }, // matte jade
			{ FLinearColor(0.92f, 0.72f, 0.69f, 1.0f), 0.00f, 0.56f, 0.34f, 0.05f }, // rose ceramic
			{ FLinearColor(0.70f, 0.73f, 0.84f, 1.0f), 0.00f, 0.40f, 0.62f, 0.03f }, // slate stone
		};

		const int32 PresetIndex = FMath::Abs(VariantIndex) % UE_ARRAY_COUNT(Presets);
		return Presets[PresetIndex];
	}

	int32 GetDefaultVariantForFaceCount(int32 FaceCount)
	{
		switch (FaceCount)
		{
		case 2:  return 1;
		case 4:  return 3;
		case 6:  return 0;
		case 8:  return 4;
		case 10: return 2;
		case 12: return 5;
		case 20: return 2;
		default: return 0;
		}
	}
}

void ACombatDicePreviewActor::InitializeMaterials()
{
	// 주사위 몸체: 무늬 없이 재질감만 보이게 쓰는 본체 머티리얼. 'Color'를 SetDiceColor로 갱신.
	// 메시 적용은 ApplyDiceMesh에서 mDiceMesh->SetMaterial(0, ...)로.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DiceBodyMaterialFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Dice/M_DiceBody.M_DiceBody"));
	if (DiceBodyMaterialFinder.Succeeded())
	{
		mDiceMaterial = UMaterialInstanceDynamic::Create(DiceBodyMaterialFinder.Object, this);
		if (mDiceMaterial != nullptr)
		{
			ApplyDiceBodyMaterialPreset();
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

	// Material-only pass: 면별 장식/테두리 텍스처 레이어는 존재하지 않는다. 숫자만 TextRender로 얹는다.
}

void ACombatDicePreviewActor::ApplyDiceBodyMaterialPreset()
{
	if (mDiceMaterial == nullptr)
	{
		return;
	}

	const int32 VariantIndex = mDiceMaterialVariantIndex != INDEX_NONE
		? mDiceMaterialVariantIndex
		: GetDefaultVariantForFaceCount(mCurrentFaceCount);
	const FDiceBodyMaterialPreset& Preset = GetDiceBodyMaterialPreset(VariantIndex);
	const float TintWeight = mCurrentFaceCount == 2 ? 0.22f : 0.26f;
	const FLinearColor BodyColor = FMath::Lerp(Preset.mBaseColor, mDiceTintColor, TintWeight);

	mDiceMaterial->SetVectorParameterValue(TEXT("Color"), BodyColor);
	// 현재 M_DiceBody가 노출하지 않는 파라미터는 무시된다. 노출되면 각 variant의 질감 차이까지 동작한다.
	mDiceMaterial->SetScalarParameterValue(TEXT("Metallic"), Preset.mMetallic);
	mDiceMaterial->SetScalarParameterValue(TEXT("Specular"), Preset.mSpecular);
	mDiceMaterial->SetScalarParameterValue(TEXT("Roughness"), Preset.mRoughness);
	mDiceMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), Preset.mEmissiveStrength);
}
