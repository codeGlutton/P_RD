#include "Actor/Dice/CombatDicePreviewActor.h"

#include "Components/TextRenderComponent.h"
#include "Engine/Font.h"
#include "Actor/Dice/CombatDicePreviewActorPrivate.h"

void ACombatDicePreviewActor::InitializeFaceTexts()
{
	constexpr float FaceTextOffset = 59.0f;

	/*
	 * Viewport 카메라는 -X 쪽에서 주사위를 바라본다.
	 * 따라서 기본 정지 자세에서는 -X 면의 숫자 5가 정면에 보이게 둔다.
	 */
	mFaceTexts.SetNum(6);
	AddFaceText(1, TEXT("FaceText_Bottom"), FVector(0.0f, 0.0f, -FaceTextOffset), RDCombatDicePreview::MakeFaceTextRotation(1));
	AddFaceText(2, TEXT("FaceText_Back"), FVector(FaceTextOffset, 0.0f, 0.0f), RDCombatDicePreview::MakeFaceTextRotation(2));
	AddFaceText(3, TEXT("FaceText_Right"), FVector(0.0f, FaceTextOffset, 0.0f), RDCombatDicePreview::MakeFaceTextRotation(3));
	AddFaceText(4, TEXT("FaceText_Left"), FVector(0.0f, -FaceTextOffset, 0.0f), RDCombatDicePreview::MakeFaceTextRotation(4));
	AddFaceText(5, TEXT("FaceText_Front"), FVector(-FaceTextOffset, 0.0f, 0.0f), RDCombatDicePreview::MakeFaceTextRotation(5));
	AddFaceText(6, TEXT("FaceText_Top"), FVector(0.0f, 0.0f, FaceTextOffset), RDCombatDicePreview::MakeFaceTextRotation(6));
}

void ACombatDicePreviewActor::AddFaceText(int32 FaceValue, const TCHAR* Name, const FVector& RelativeLocation, const FRotator& RelativeRotation)
{
	UTextRenderComponent* FaceTextComponent = CreateDefaultSubobject<UTextRenderComponent>(Name);
	FaceTextComponent->SetupAttachment(mDiceRoot);
	if (mDiceNumberFont != nullptr)
	{
		FaceTextComponent->SetFont(mDiceNumberFont);
	}
	FaceTextComponent->SetHorizontalAlignment(EHTA_Center);
	FaceTextComponent->SetVerticalAlignment(EVRTA_TextCenter);
	FaceTextComponent->SetWorldSize(58.0f);
	FaceTextComponent->SetTextRenderColor(FColor(8, 16, 16, 255));
	FaceTextComponent->SetRelativeLocation(RelativeLocation);
	FaceTextComponent->SetRelativeRotation(RelativeRotation);
	FaceTextComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (mFaceTexts.IsValidIndex(FaceValue - 1))
	{
		mFaceTexts[FaceValue - 1] = FaceTextComponent;
	}
}

void ACombatDicePreviewActor::SetFaceText(int32 FaceValue, const FText& FaceText)
{
	if (mFaceTexts.IsValidIndex(FaceValue - 1) && mFaceTexts[FaceValue - 1] != nullptr)
	{
		mFaceTexts[FaceValue - 1]->SetText(FaceText);
	}
}
