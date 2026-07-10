#include "UI/DicePanelWidget.h"

#include "Components/Button.h"

UDicePanelWidget::UDicePanelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 이 에셋은 SVN 미연동 환경 등에서 파일이 없을 수 있으므로 LoadObject로 안전하게 로드합니다.
	mDicePanelCardTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Common/T_UI_Dice_Slot_Empty.T_UI_Dice_Slot_Empty"));
}

void UDicePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UDicePanelWidget::NativeDestruct()
{
	if (mDiceCarouselPreviousButton != nullptr)
	{
		mDiceCarouselPreviousButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselPreviousButtonClicked);
	}
	if (mDiceCarouselNextButton != nullptr)
	{
		mDiceCarouselNextButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselNextButtonClicked);
	}
	if (mDiceCarouselBackButton != nullptr)
	{
		mDiceCarouselBackButton->OnClicked.RemoveDynamic(this, &UDicePanelWidget::HandleDiceCarouselBackButtonClicked);
	}
	UnbindDiceCarouselFaceButtons();

	DestroyDicePanelCaptureActors();

	Super::NativeDestruct();
}

void UDicePanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickDicePanelFaceRotation(InDeltaTime);

	if (mPendingDicePanelCaptureIndex != INDEX_NONE)
	{
		const int32 CaptureIndex = mPendingDicePanelCaptureIndex;
		mPendingDicePanelCaptureIndex = INDEX_NONE;
		CaptureDicePanelPreviewActor(CaptureIndex);
	}
}

void UDicePanelWidget::ApplyOpenUI()
{
	RefreshDicePanelViews();
	mDicePanelCarouselActivated = false;
	mSelectedDicePanelIndex = mDicePanelViews.IsEmpty() ? INDEX_NONE : 0;
	mPressedDicePanelIndex = INDEX_NONE;
	mDicePanelPressPosition = FVector2D::ZeroVector;
	mPendingDicePanelCaptureIndex = INDEX_NONE;
	mDicePanelPreviewRotations.SetNum(DicePanelSlotCount);
	mDicePanelTargetPreviewRotations.SetNum(DicePanelSlotCount);
	mDicePanelPreviewRotationAnimations.SetNum(DicePanelSlotCount);
	mDicePanelSelectedFaceValues.SetNum(DicePanelSlotCount);
	for (int32 DiceIndex = 0; DiceIndex < DicePanelSlotCount; ++DiceIndex)
	{
		mDicePanelPreviewRotations[DiceIndex] = GetDicePanelIdleRotation(DiceIndex).Quaternion();
		mDicePanelTargetPreviewRotations[DiceIndex] = mDicePanelPreviewRotations[DiceIndex];
		mDicePanelPreviewRotationAnimations[DiceIndex] = false;
		mDicePanelSelectedFaceValues[DiceIndex] = INDEX_NONE;
	}

	Super::ApplyOpenUI();

	/*
	 * 기존 WBP의 2D 카드 슬롯은 숨기고, 루트 캔버스 위에 3D 주사위 캡처 이미지를 새로 올린다.
	 * 2D 숫자 카드용 크기/터치 판정에 그대로 끼워 넣으면 주사위가 잘리거나 입력이 어긋나기 때문이다.
	 */
	EnsureDicePanelPreviewWidgets();
	EnsureDiceCarouselControlWidgets();
	RefreshDicePanelWidgets();
	RefreshDiceCarouselControlWidgets();
}

/** @brief 주사위 이미지는 카드 자체가 아니라 캡처 액터 회전값으로만 기울인다. */
// 부모 캐러셀의 2D 카드 회전을 쓰면 3D 주사위가 아니라 이미지 사각형이 기울어져 보인다.
// 그래서 이 패널에서는 캐러셀 카드 각도는 0으로 고정하고, 실제 주사위 액터 회전만 별도로 갱신한다.
float UDicePanelWidget::GetCarouselItemAngle(int32 ItemIndex) const
{
	return 0.0f;
}

int32 UDicePanelWidget::GetCarouselItemDisplayCount() const
{
	return FMath::Min(mDicePanelViews.Num(), DicePanelSlotCount);
}

/** @brief 다른 주사위 카드를 선택하면 상세 카드 배치와 캡처 이미지를 갱신한다. */
// 현재 선택 index에 따라 큰 주사위/좌우 주사위 배치가 달라지므로,
// 선택 변경 시 런타임 Image 위치와 RenderTarget 캡처를 함께 갱신한다.
void UDicePanelWidget::HandleCarouselSelectionChanged(int32 PreviousIndex, int32 NewIndex)
{
	RefreshDicePanelWidgets();
	RefreshDicePanelPreviewActors();
}
