#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"

/** @brief 주사위 루트 회전만 바꿔 메시/숫자/면 커버가 함께 회전하게 한다. */
void ACombatDicePreviewActor::SetDiceRotation(const FRotator& NewRotation)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetRelativeRotation(NewRotation);
	}
}

/** @brief 물리 면 0-base 순서의 값 배열을 1-base FaceText API로 전달한다. */
void ACombatDicePreviewActor::SetFaceValues(const TArray<int32>& NewFaceValues)
{
	for (int32 FaceIndex = 0; FaceIndex < NewFaceValues.Num(); ++FaceIndex)
	{
		SetFaceText(FaceIndex + 1, FText::AsNumber(NewFaceValues[FaceIndex]));
	}
}

/** @brief 물리 면 0-base 순서의 텍스처 override를 각 면 커버 머티리얼에 적용한다. */
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

/** @brief 명시 텍스처가 없으면 현재 면 수 기본 커버, 마지막으로 공통 fallback 텍스처를 반환한다. */
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

/** @brief 면 값과 면 텍스처를 같은 물리 면 순서로 한 번에 갱신한다. */
void ACombatDicePreviewActor::SetFaceData(const TArray<int32>& NewFaceValues, const TArray<TObjectPtr<UTexture>>& NewFaceTextures)
{
	SetFaceValues(NewFaceValues);
	SetFaceTextures(NewFaceTextures);
}

/** @brief 지정 눈 값이 정면으로 보이는 정지 자세를 즉시 적용한다. */
void ACombatDicePreviewActor::SettleToFace(int32 FaceValue)
{
	if (mDiceRoot != nullptr)
	{
		mDiceRoot->SetRelativeRotation(GetSettledFaceRotation(FaceValue));
	}
}

/** @brief 희귀도/상태 색을 밝은 몸체색과 섞어 숫자 가독성을 유지한다. */
void ACombatDicePreviewActor::SetDiceColor(const FLinearColor& NewColor)
{
	if (mDiceMaterial != nullptr)
	{
		// 어두운 숫자 가독성을 위해 몸체는 늘 밝게 유지하고, 상태 색(희귀도/사용 등)은 은은하게만 섞는다.
		// d2는 얇은 코인이라 본체/림 대비가 없으면 캡처에서 형태가 사라진다.
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

/** @brief Viewport 방식 프리뷰에서만 쓰는 배경판 표시를 토글한다. CaptureActor는 보통 숨긴다. */
void ACombatDicePreviewActor::SetBackdropVisible(bool bVisible)
{
	if (mBackdropMesh != nullptr)
	{
		mBackdropMesh->SetVisibility(bVisible);
	}
}
