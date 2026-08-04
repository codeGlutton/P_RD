#include "UI/Combat/CombatLayoutHUDWidget.h"

#include "GameFramework/Actor.h"   // FUnitUI.mViewActor->GetActorLocation() (HP바 라이브 투영)
#include "Camera/PlayerCameraManager.h"   // 카메라 줌(OrthoWidth) 비례 HP바 스케일
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Combat/CombatUIModel.h"
#include "GameplayTagType.h"   // EffectTags::GameplayEffect_StatusEffect_* (상태이상 태그→아이콘 매핑)

namespace
{
	// (WBP 클래스/채움 텍스처는 UCombatLayoutHUDWidget 생성자에서 ConstructorHelpers로 하드 레퍼런스한다 — #300 컨벤션.)

	// HP바는 기존 크기를 유지한다. 상태이상만 아래의 독립 레이아웃 값으로 키운다.
	constexpr float UnitHpBarRenderScale = 0.52f;
	// 모바일 전장에서는 월드 위 상태 아이콘이 유닛과 타일을 가리므로 표시하지 않는다.
	// 상태 데이터와 용병/몬스터 상세 패널 표시는 그대로 유지한다.
	constexpr bool bShowUnitHpBarStatusIcons = false;
	// 줌 배율 기준이 되는 직교 카메라 폭(CombatCameraPawn 기본 OrthoWidth). 이 값일 때 배율 1.0.
	constexpr float UnitHpBarBaseOrthoWidth = 2000.0f;

	// 머리 위 HP바가 유닛 머리에서 얼마나 위로 뜰지(뷰포트 픽셀). 크기 바꾸면 같이 조절.
	constexpr float UnitHpBarHeadOffsetY = -78.0f;

	// HP 숫자 폰트 크기(WBP 디자인 좌표 기준 — 렌더 스케일이 곱해져 화면에 보임). 크게 해서 바를 꽉 채운다.
	constexpr float UnitHpBarValueFontSize = 38.0f;

	// 모바일에서는 한 유닛당 세 개를 크게 보여 주고 나머지는 +N으로 압축한다.
	// 활성 개수에 맞춰 매 프레임 가운데 정렬하므로 상태 하나가 HP바 왼쪽에 떨어지지 않는다.
	constexpr int32 UnitHpBarVisibleStatusSlots = 3;
	constexpr float UnitHpBarDesignWidth = 360.0f;
	constexpr float UnitHpBarStatusFrameSize = 176.0f;   // 얇은 원형 개별 프레임
	constexpr float UnitHpBarStatusIconSize = 156.0f;    // 프레임 안을 크게 채우는 아이콘
	constexpr float UnitHpBarStatusIconStep = 184.0f;    // 아이콘 사이 최소 여백
	constexpr float UnitHpBarStatusRowTop = 72.0f;       // HP바와 시각적으로 분리
	constexpr float UnitHpBarStatusCountFontSize = 42.0f;

	float StatusRowLeft(const int32 VisibleCount)
	{
		const int32 SafeCount = FMath::Max(VisibleCount, 1);
		const float RowWidth = UnitHpBarStatusFrameSize
			+ StaticCast<float>(SafeCount - 1) * UnitHpBarStatusIconStep;
		return (UnitHpBarDesignWidth - RowWidth) * 0.5f;
	}

}

/**
 * @brief HP 채움 드레인을 준비한다: 채움 이미지 슬롯을 좌상단 기준으로 두고 원본 폭을 기록한다.
 * @details ClipToBounds 클립 캔버스로 "크롭"하는 방식은 유닛 바에 렌더 스케일(변환)을 걸면
 *          모바일 Vulkan에서 스텐실 클립 파이프라인이 없어 렌더 스레드가 죽는다(실측).
 *          그래서 클립 대신 채움 이미지의 슬롯 폭을 % 만큼 줄여 왼쪽 기준으로 비운다(솔리드 채움이라 크롭과 시각적으로 동일).
 */
void UCombatLayoutHUDWidget::SetupUnitHpBarFillClip(FCombatUnitHpBarWidget& Bar)
{
	if (Bar.mFillImage == nullptr)
	{
		return;
	}

	UCanvasPanelSlot* FillSlot = Cast<UCanvasPanelSlot>(Bar.mFillImage->Slot);
	if (FillSlot == nullptr)
	{
		return;
	}

	// 좌상단(0,0) 기준으로 두면, 폭을 줄일 때 왼쪽은 고정되고 오른쪽부터 비어 HP 드레인처럼 보인다.
	FillSlot->SetAlignment(FVector2D(0.0f, 0.0f));
	FillSlot->SetAutoSize(false);
	Bar.mFillFullWidth = FillSlot->GetSize().X;
	Bar.mFillClipSlot = FillSlot;   // Update에서 폭을 % 만큼 조절할 대상(채움 이미지 자체 슬롯).
}

/** @brief WBP의 상태 슬롯(StatusIcon_0N/StatusCountText_0N/Overflow)을 캐시하고 기본은 숨김으로 둔다. */
void UCombatLayoutHUDWidget::CacheUnitHpBarStatusSlots(FCombatUnitHpBarWidget& Bar) const
{
	if (Bar.mRoot == nullptr)
	{
		return;
	}

	Bar.mStatusIcons.Reset();
	Bar.mStatusCountTexts.Reset();
	for (int32 SlotIndex = 1; SlotIndex <= 5; ++SlotIndex)
	{
		UImage* Icon = Cast<UImage>(Bar.mRoot->GetWidgetFromName(FName(*FString::Printf(TEXT("StatusIcon_%02d"), SlotIndex))));
		UTextBlock* Count = Cast<UTextBlock>(Bar.mRoot->GetWidgetFromName(FName(*FString::Printf(TEXT("StatusCountText_%02d"), SlotIndex))));
		if (SlotIndex > UnitHpBarVisibleStatusSlots)
		{
			if (Icon != nullptr) { Icon->SetVisibility(ESlateVisibility::Collapsed); }
			if (Count != nullptr) { Count->SetVisibility(ESlateVisibility::Collapsed); }
			continue;
		}
		Bar.mStatusIcons.Add(Icon);
		Bar.mStatusCountTexts.Add(Count);

		const float SlotX = StatusRowLeft(UnitHpBarVisibleStatusSlots)
			+ StaticCast<float>(SlotIndex - 1) * UnitHpBarStatusIconStep;
		const float IconInset = (UnitHpBarStatusFrameSize - UnitHpBarStatusIconSize) * 0.5f;
		// 아이콘: 크게 + 한 줄로 재배치(WBP 기본 24px은 렌더스케일에서 안 보임).
		if (Icon != nullptr)
		{
			if (UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
			{
				IconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				IconSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				IconSlot->SetPosition(FVector2D(SlotX + IconInset, UnitHpBarStatusRowTop + IconInset));
				IconSlot->SetSize(FVector2D(UnitHpBarStatusIconSize, UnitHpBarStatusIconSize));
				IconSlot->SetAutoSize(false);
			}
			Icon->SetColorAndOpacity(FLinearColor::White);   // 투명도 제거(불투명, 틴트 없음).
			Icon->SetRenderOpacity(1.0f);
			Icon->SetVisibility(ESlateVisibility::Collapsed);     // 기본 숨김(데이터 있을 때만 켠다).
		}
		// 개수 텍스트: 아이콘 우하단에 겹쳐서 크게 + 외곽선.
		if (Count != nullptr)
		{
			FSlateFontInfo CountFont = Count->GetFont();
			CountFont.Size = UnitHpBarStatusCountFontSize;
			CountFont.OutlineSettings.OutlineSize = 3;
			CountFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
			Count->SetFont(CountFont);
			Count->SetJustification(ETextJustify::Right);
			Count->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			Count->SetRenderOpacity(1.0f);   // 투명도 제거(불투명).
			if (UCanvasPanelSlot* CountSlot = Cast<UCanvasPanelSlot>(Count->Slot))
			{
				CountSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				CountSlot->SetAlignment(FVector2D(0.0f, 0.0f));
				CountSlot->SetPosition(FVector2D(SlotX + IconInset + UnitHpBarStatusIconSize * 0.35f,
					UnitHpBarStatusRowTop + IconInset + UnitHpBarStatusIconSize * 0.42f));
				CountSlot->SetSize(FVector2D(UnitHpBarStatusIconSize * 0.65f, UnitHpBarStatusIconSize * 0.58f));
				CountSlot->SetAutoSize(false);
			}
			Count->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	Bar.mStatusOverflowText = Cast<UTextBlock>(Bar.mRoot->GetWidgetFromName(TEXT("StatusOverflowText")));
	if (Bar.mStatusOverflowText != nullptr)
	{
		FSlateFontInfo OverflowFont = Bar.mStatusOverflowText->GetFont();
		OverflowFont.Size = UnitHpBarStatusCountFontSize;
		OverflowFont.OutlineSettings.OutlineSize = 3;
		OverflowFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
		Bar.mStatusOverflowText->SetFont(OverflowFont);
		Bar.mStatusOverflowText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCombatLayoutHUDWidget::RebuildUnitHpBars()
{
	if (mRootCanvas == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// (mUnitHpBarWidgetClass / mUnitHpFill{Red,Green}Texture 는 생성자에서 하드 레퍼런스로 이미 로드됨 — #300 컨벤션.)

	const int32 DesiredCount = mUIModel != nullptr ? mUIModel->GetUnitUIs().Num() : 0;

	/*
	 * HP바는 **판이 그린 것 전부보다 뒤**에 깔린다.
	 *
	 * 옛 HUD 는 겹이 둘(풀뷰포트 + 디자인 캔버스)이라 디자인 캔버스보다 낮은
	 * 값을 골랐다. 새 HUD 는 캔버스가 하나뿐이라 그 기준이 없어졌는데 값만
	 * 남아 있었다 -- WBP 가 그린 카드와 아군 칸이 0 근처라, 5 는 그것들보다
	 * **위**였다. 머리 위 바가 스킬 카드를 덮었다.
	 *
	 * 음수로 내려 판이 그린 것 아래로 확실히 보낸다.
	 */
	const int32 UnitHpBarZOrder = -10;


	// 유닛 수보다 많은 바는 제거.
	for (int32 BarIndex = mUnitHpBars.Num() - 1; BarIndex >= DesiredCount; --BarIndex)
	{
		if (mUnitHpBars[BarIndex].mRoot != nullptr)
		{
			mUnitHpBars[BarIndex].mRoot->RemoveFromParent();
		}
		mUnitHpBars.RemoveAt(BarIndex);
	}

	// 모자란 바는 새로 만든다.
	for (int32 BarIndex = mUnitHpBars.Num(); BarIndex < DesiredCount; ++BarIndex)
	{
		FCombatUnitHpBarWidget NewBar;
		if (mUnitHpBarWidgetClass != nullptr)
		{
			NewBar.mRoot = CreateWidget<UUserWidget>(this, mUnitHpBarWidgetClass);
		}
		if (NewBar.mRoot == nullptr)
		{
			continue;
		}

		NewBar.mRoot->SetVisibility(ESlateVisibility::Collapsed);

		// named widget 캐시 + 트리 수술(상태 슬롯 숨김, 채움 크롭 래핑)은
		// 반드시 AddChildToCanvas(Slate 트리 생성) "이전"에 끝낸다.
		// 라이브 트리에 붙인 뒤 UMG WidgetTree를 재부모화하면 Slate 트리와 어긋나 렌더 크래시가 난다.
		NewBar.mFillImage = Cast<UImage>(NewBar.mRoot->GetWidgetFromName(TEXT("HpFillImage")));
		NewBar.mValueText = Cast<UTextBlock>(NewBar.mRoot->GetWidgetFromName(TEXT("HpValueText")));
		CacheUnitHpBarStatusSlots(NewBar);   // 상태칸 캐시+숨김. 실제 표시는 UpdateUnitHpBarStatus에서 DTO로 매 프레임 채운다.
		SetupUnitHpBarFillClip(NewBar);

		// HP 숫자: 크게 + 외곽선(두껍게/대비) + 바(채움) 폭에 맞춰 가운데 정렬 → 작아도 잘 보이게.
		if (NewBar.mValueText != nullptr)
		{
			FSlateFontInfo ValueFont = NewBar.mValueText->GetFont();
			ValueFont.Size = UnitHpBarValueFontSize;
			ValueFont.OutlineSettings.OutlineSize = 3;
			ValueFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
			NewBar.mValueText->SetFont(ValueFont);
			NewBar.mValueText->SetJustification(ETextJustify::Center);
			NewBar.mValueText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			if (UCanvasPanelSlot* ValueSlot = Cast<UCanvasPanelSlot>(NewBar.mValueText->Slot))
			{
				// WBP 실측: 채움 영역 (x=20, y=8, w=323, h=53). 그 폭에 맞춰 가로로 꽉, 세로 중앙 근처.
				ValueSlot->SetAnchors(FAnchors(0.0f, 0.0f));
				ValueSlot->SetAlignment(FVector2D(0.0f, 0.5f));
				ValueSlot->SetPosition(FVector2D(20.0f, 34.0f));
				ValueSlot->SetSize(FVector2D(323.0f, 44.0f));
				ValueSlot->SetAutoSize(false);
			}
		}

		// HP바 왼쪽 방어도 표시(아이콘 + 수치). WBP 내부 캔버스에 런타임 생성 — 트리 수술 규칙에 따라
		// 반드시 AddChildToCanvas(라이브 부착) 전에 만든다. 방어도 0이면 Update에서 숨긴다.
		if (UWidgetTree* BarTree = NewBar.mRoot->WidgetTree)
		{
			if (UCanvasPanel* BarCanvas = Cast<UCanvasPanel>(BarTree->RootWidget))
			{
				if (bShowUnitHpBarStatusIcons && mUnitStatusSlotTexture != nullptr)
				{
					NewBar.mStatusFrames.Reset();
					for (int32 SlotIndex = 0; SlotIndex < UnitHpBarVisibleStatusSlots; ++SlotIndex)
					{
						UImage* StatusFrame = BarTree->ConstructWidget<UImage>(UImage::StaticClass(),
							FName(*FString::Printf(TEXT("StatusFrame_%02d"), SlotIndex + 1)));
						if (StatusFrame == nullptr)
						{
							NewBar.mStatusFrames.Add(nullptr);
							continue;
						}
						StatusFrame->SetBrushFromTexture(mUnitStatusSlotTexture, false);
						StatusFrame->SetVisibility(ESlateVisibility::Collapsed);
						if (UCanvasPanelSlot* FrameSlot = BarCanvas->AddChildToCanvas(StatusFrame))
						{
							FrameSlot->SetAnchors(FAnchors(0.0f, 0.0f));
							FrameSlot->SetAlignment(FVector2D(0.0f, 0.0f));
							FrameSlot->SetPosition(FVector2D(
								StatusRowLeft(UnitHpBarVisibleStatusSlots) + SlotIndex * UnitHpBarStatusIconStep,
								UnitHpBarStatusRowTop));
							FrameSlot->SetSize(FVector2D(UnitHpBarStatusFrameSize, UnitHpBarStatusFrameSize));
							FrameSlot->SetAutoSize(false);
							FrameSlot->SetZOrder(-5);
						}
						NewBar.mStatusFrames.Add(StatusFrame);
					}
				}
				if (mUnitDefenseIconTexture != nullptr)
				{
					if (UImage* DefenseIcon = BarTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DefenseIcon")))
					{
						DefenseIcon->SetBrushFromTexture(mUnitDefenseIconTexture, false);
						DefenseIcon->SetVisibility(ESlateVisibility::Collapsed);
						if (UCanvasPanelSlot* IconSlot = BarCanvas->AddChildToCanvas(DefenseIcon))
						{
							// WBP 실측(채움 x=20,y=8,w=323,h=53) 기준, 바 왼쪽 바깥에 세로 중앙 정렬.
							IconSlot->SetAnchors(FAnchors(0.0f, 0.0f));
							IconSlot->SetAlignment(FVector2D(1.0f, 0.5f));
							IconSlot->SetPosition(FVector2D(14.0f, 34.0f));
							IconSlot->SetSize(FVector2D(64.0f, 64.0f));
							IconSlot->SetAutoSize(false);
						}
						NewBar.mDefenseIcon = DefenseIcon;
					}
				}
				if (UTextBlock* DefenseText = BarTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DefenseValueText")))
				{
					FSlateFontInfo DefenseFont = DefenseText->GetFont();
					DefenseFont.Size = 26;
					DefenseFont.OutlineSettings.OutlineSize = 3;
					DefenseFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.9f);
					DefenseText->SetFont(DefenseFont);
					DefenseText->SetJustification(ETextJustify::Center);
					DefenseText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					DefenseText->SetVisibility(ESlateVisibility::Collapsed);
					if (UCanvasPanelSlot* TextSlot = BarCanvas->AddChildToCanvas(DefenseText))
					{
						// 아이콘(방패) 위 중앙에 수치를 겹쳐 그린다.
						TextSlot->SetAnchors(FAnchors(0.0f, 0.0f));
						TextSlot->SetAlignment(FVector2D(1.0f, 0.5f));
						TextSlot->SetPosition(FVector2D(10.0f, 34.0f));
						TextSlot->SetSize(FVector2D(56.0f, 40.0f));
						TextSlot->SetAutoSize(false);
						TextSlot->SetZOrder(1);
					}
					NewBar.mDefenseText = DefenseText;
				}
			}
		}

		// 크기 축소: 렌더 스케일을 하단 중앙 피벗으로 적용해, 축소해도 바 밑변이 투영 지점(머리 위)에 고정되게 한다.
		NewBar.mRoot->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
		NewBar.mRoot->SetRenderScale(FVector2D(UnitHpBarRenderScale, UnitHpBarRenderScale));

		// [Adreno Vulkan RHIThread 크래시 방지] 위 렌더 스케일(변환)이 걸린 HP바 서브트리 안에 ClipToBounds 위젯이 있으면
		// 모바일 Vulkan에서 스텐실-클립 파이프라인이 없어 vkCmdBindPipeline(null)로 렌더 스레드가 즉사한다(실측).
		// (기존 대응은 채움 크롭의 클립만 슬롯폭으로 우회 — 변환은 남아있음.) 변환 서브트리의 나머지 클립도 강제로 끈다.
		// 솔리드 채움/아이콘/텍스트라 클립이 없어도 시각 차이는 거의 없다.
		NewBar.mRoot->SetClipping(EWidgetClipping::Inherit);
		if (UWidgetTree* BarWidgetTree = NewBar.mRoot->WidgetTree)
		{
			BarWidgetTree->ForEachWidget([](UWidget* ChildWidget)
			{
				if (ChildWidget != nullptr && ChildWidget->GetClipping() == EWidgetClipping::ClipToBounds)
				{
					ChildWidget->SetClipping(EWidgetClipping::Inherit);
				}
			});
		}

		// 트리 확정 후 라이브 캔버스에 붙인다.
		mRootCanvas->AddChildToCanvas(NewBar.mRoot);
		if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(NewBar.mRoot->Slot))
		{
			RootSlot->SetAutoSize(true);                      // WBP가 정한 자체 크기를 사용.
			RootSlot->SetAlignment(FVector2D(0.5f, 1.0f));    // 가로 중앙, 세로 아래 기준(유닛 머리 위에 놓기).
			RootSlot->SetZOrder(UnitHpBarZOrder);             // 스킨 뒤로(다른 UI를 안 덮게).
		}

		mUnitHpBars.Add(NewBar);
	}

	UpdateUnitHpBars();
}

void UCombatLayoutHUDWidget::UpdateUnitHpBars()
{
	// 지도(풀스크린) 열림 중에는 유닛 머리 위 HP바를 숨긴다 — 탑바만 남기는 뷰.
	// 이 함수가 매 틱 HP바를 강제 표시(라인 283)하므로, SetCombatPlayControlsVisible에서 한 번 숨기는 것으로는
	// 다음 틱에 되살아난다. 여기서 게이트해야 지도 뷰 동안 계속 숨겨진다.
	if (GetVisibility() == ESlateVisibility::Collapsed)
	{
		for (FCombatUnitHpBarWidget& Bar : mUnitHpBars)
		{
			if (Bar.mRoot != nullptr) { Bar.mRoot->SetVisibility(ESlateVisibility::Collapsed); }
		}
		return;
	}

	if (mUIModel == nullptr)
	{
		return;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		return;
	}

	// 카메라 줌(직교 OrthoWidth)에 비례해 HP바 크기를 키운다 — 줌인하면 유닛과 함께 바도 커진다.
	// 기준 OrthoWidth(2000) 대비 배율, 과도한 확대/축소는 클램프.
	float ZoomScale = 1.0f;
	if (PlayerController->PlayerCameraManager != nullptr)
	{
		const float CurrentOrthoWidth = PlayerController->PlayerCameraManager->GetCameraCacheView().OrthoWidth;
		if (CurrentOrthoWidth > KINDA_SMALL_NUMBER)
		{
			ZoomScale = FMath::Clamp(UnitHpBarBaseOrthoWidth / CurrentOrthoWidth, 0.6f, 3.0f);
		}
	}
	const FVector2D BarRenderScale(UnitHpBarRenderScale * ZoomScale, UnitHpBarRenderScale * ZoomScale);

	const TArray<FUnitUI>& Units = mUIModel->GetUnitUIs();
	for (int32 BarIndex = 0; BarIndex < mUnitHpBars.Num(); ++BarIndex)
	{
		FCombatUnitHpBarWidget& Bar = mUnitHpBars[BarIndex];
		if (Bar.mRoot == nullptr || Units.IsValidIndex(BarIndex) == false)
		{
			continue;
		}
		const FUnitUI& Unit = Units[BarIndex];

		const float Percent = Unit.mMaxHP > 0.0f ? FMath::Clamp(Unit.mHP / Unit.mMaxHP, 0.0f, 1.0f) : 0.0f;

		// 채움 색: 아군=초록, 적=빨강 텍스처. (없으면 기존 브러시 유지)
		if (Bar.mFillImage != nullptr)
		{
			UTexture2D* FillTexture = Unit.mIsPlayer ? mUnitHpFillGreenTexture : mUnitHpFillRedTexture;
			if (FillTexture != nullptr)
			{
				Bar.mFillImage->SetBrushResourceObject(FillTexture);
			}
		}

		// 채움 드레인: 채움 이미지 슬롯 폭을 원본 폭 × 퍼센트로 줄인다(좌상단 고정이라 오른쪽부터 HP가 빠진다).
		if (Bar.mFillClipSlot != nullptr && Bar.mFillFullWidth > 0.0f)
		{
			FVector2D FillSize = Bar.mFillClipSlot->GetSize();
			FillSize.X = Bar.mFillFullWidth * Percent;
			Bar.mFillClipSlot->SetSize(FillSize);
		}

		// HP 숫자: 모바일에서 현재치만 보면 최대치와 피해 비율을 다시
		// 추론해야 한다. 바 안에 현재/최대를 같이 크게 표시한다.
		if (Bar.mValueText != nullptr)
		{
			Bar.mValueText->SetText(FText::FromString(FString::Printf(
				TEXT("%d/%d"), FMath::RoundToInt(Unit.mHP),
				FMath::RoundToInt(Unit.mMaxHP))));
		}

		// 전투 HUD의 공개 수치는 HP/AP/속도뿐이다. 구형 방어도 슬롯이
		// 남아 있더라도 월드 HP바에서는 항상 감춘다.
		const ESlateVisibility DefenseVisibility = ESlateVisibility::Collapsed;
		if (Bar.mDefenseIcon != nullptr)
		{
			Bar.mDefenseIcon->SetVisibility(DefenseVisibility);
		}
		if (Bar.mDefenseText != nullptr)
		{
			Bar.mDefenseText->SetVisibility(DefenseVisibility);
		}

		// 유닛 월드 위치를 화면(위젯) 좌표로 투영. 화면 밖이면 숨긴다.
		// 이동을 매 프레임 따라가도록 뷰 액터의 라이브 위치를 우선 투영(유효 시). 없으면 스냅샷(mWorldLocation) 폴백.
		const FVector ProjectLocation = Unit.mViewActor.IsValid()
			? Unit.mViewActor->GetActorLocation()
			: Unit.mWorldLocation;
		FVector2D ScreenPosition;
		const bool bOnScreen = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
			PlayerController, ProjectLocation, ScreenPosition, false);
		if (bOnScreen == false)
		{
			Bar.mRoot->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* RootSlot = Cast<UCanvasPanelSlot>(Bar.mRoot->Slot))
		{
			RootSlot->SetPosition(ScreenPosition + FVector2D(0.0f, UnitHpBarHeadOffsetY));   // 유닛 머리 위로 띄운다.
		}

		// 줌 배율 반영(하단 중앙 피벗이라 커져도 밑변이 머리 위 투영점에 고정).
		Bar.mRoot->SetRenderScale(BarRenderScale);

		// HP바 밑 상태이상 아이콘/개수 갱신(온스크린 확정된 바에만).
		UpdateUnitHpBarStatus(Bar, Unit);

		Bar.mRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

/** @brief 상태이상 태그 → 아이콘 텍스처. 로그용 아이콘(mLogIcon*)을 재사용한다. 전용 아이콘 없는 태그는 nullptr. */
UTexture2D* UCombatLayoutHUDWidget::ResolveStatusIcon(const FGameplayTag& StatusTag) const
{
	if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Agility))        { return mLogIconAgility; }
	if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Buff_Fortification))  { return mLogIconFortification; }
	if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Vulnerability)) { return mLogIconVulnerability; }
	if (StatusTag.MatchesTag(EffectTags::GameplayEffect_StatusEffect_TurnDuration_Debuff_Weakness))     { return mLogIconWeakness; }
	return nullptr;
}

/**
 * @brief 유닛 HP바 밑 상태이상 칸(큰 아이콘 3개 + 개수 + 오버플로)을 DTO(mStatusEffects)로 채운다.
 * @details 아이콘 있는 상태만 가운데 정렬하고, 3칸 초과분은 StatusOverflowText에 "+N". 스택 2 이상만 개수 표시.
 */
void UCombatLayoutHUDWidget::UpdateUnitHpBarStatus(FCombatUnitHpBarWidget& Bar, const FUnitUI& Unit) const
{
	if (bShowUnitHpBarStatusIcons == false)
	{
		for (UImage* Icon : Bar.mStatusIcons)
		{
			if (Icon != nullptr) { Icon->SetVisibility(ESlateVisibility::Collapsed); }
		}
		for (UImage* Frame : Bar.mStatusFrames)
		{
			if (Frame != nullptr) { Frame->SetVisibility(ESlateVisibility::Collapsed); }
		}
		for (UTextBlock* Count : Bar.mStatusCountTexts)
		{
			if (Count != nullptr) { Count->SetVisibility(ESlateVisibility::Collapsed); }
		}
		if (Bar.mStatusOverflowText != nullptr)
		{
			Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	const int32 SlotCount = Bar.mStatusIcons.Num();

	int32 Shown = 0;
	int32 Iconable = 0;
	for (const FStatusEffectUI& Status : Unit.mStatusEffects)
	{
		if (ResolveStatusIcon(Status.mTag) != nullptr)
		{
			++Iconable;
		}
	}

	const int32 VisibleCount = FMath::Min(Iconable, SlotCount);
	const float RowLeft = StatusRowLeft(VisibleCount);
	const float IconInset = (UnitHpBarStatusFrameSize - UnitHpBarStatusIconSize) * 0.5f;
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const float SlotX = RowLeft + StaticCast<float>(SlotIndex) * UnitHpBarStatusIconStep;
		if (UImage* IconImage = Bar.mStatusIcons[SlotIndex])
		{
			if (UCanvasPanelSlot* IconSlot = Cast<UCanvasPanelSlot>(IconImage->Slot))
			{
				IconSlot->SetPosition(FVector2D(SlotX + IconInset, UnitHpBarStatusRowTop + IconInset));
			}
		}
		if (Bar.mStatusCountTexts.IsValidIndex(SlotIndex))
		{
			if (UTextBlock* CountText = Bar.mStatusCountTexts[SlotIndex])
			{
				if (UCanvasPanelSlot* CountSlot = Cast<UCanvasPanelSlot>(CountText->Slot))
				{
					CountSlot->SetPosition(FVector2D(
						SlotX + IconInset + UnitHpBarStatusIconSize * 0.35f,
						UnitHpBarStatusRowTop + IconInset + UnitHpBarStatusIconSize * 0.42f));
				}
			}
		}
		if (Bar.mStatusFrames.IsValidIndex(SlotIndex))
		{
			if (UImage* StatusFrame = Bar.mStatusFrames[SlotIndex])
			{
				if (UCanvasPanelSlot* FrameSlot = Cast<UCanvasPanelSlot>(StatusFrame->Slot))
				{
					FrameSlot->SetPosition(FVector2D(SlotX, UnitHpBarStatusRowTop));
				}
			}
		}
	}

	for (const FStatusEffectUI& Status : Unit.mStatusEffects)
	{
		UTexture2D* Icon = ResolveStatusIcon(Status.mTag);
		if (Icon == nullptr)
		{
			continue;   // 전용 아이콘 없는 상태는 표시 생략.
		}
		if (Shown >= SlotCount)
		{
			continue;   // 슬롯 초과분은 아래 오버플로로만 집계.
		}

		if (UImage* IconImage = Bar.mStatusIcons[Shown])
		{
			IconImage->SetBrushResourceObject(Icon);
			IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		if (Bar.mStatusCountTexts.IsValidIndex(Shown))
		{
			if (UTextBlock* CountText = Bar.mStatusCountTexts[Shown])
			{
				if (Status.mStackCount > 1)
				{
					CountText->SetText(FText::AsNumber(Status.mStackCount));
					CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					CountText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		++Shown;
	}

	// 채우지 못한 뒤쪽 슬롯 숨김.
	for (int32 SlotIndex = Shown; SlotIndex < SlotCount; ++SlotIndex)
	{
		if (UImage* IconImage = Bar.mStatusIcons[SlotIndex]) { IconImage->SetVisibility(ESlateVisibility::Collapsed); }
	}
	for (int32 SlotIndex = Shown; SlotIndex < Bar.mStatusCountTexts.Num(); ++SlotIndex)
	{
		if (UTextBlock* CountText = Bar.mStatusCountTexts[SlotIndex]) { CountText->SetVisibility(ESlateVisibility::Collapsed); }
	}
	for (int32 SlotIndex = 0; SlotIndex < Bar.mStatusFrames.Num(); ++SlotIndex)
	{
		if (UImage* StatusFrame = Bar.mStatusFrames[SlotIndex])
		{
			StatusFrame->SetVisibility(SlotIndex < Shown
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
		}
	}

	// 세 칸 초과 상태 개수를 "+N"으로.
	if (Bar.mStatusOverflowText != nullptr)
	{
		const int32 Overflow = Iconable - SlotCount;
		if (Overflow > 0)
		{
			Bar.mStatusOverflowText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), Overflow)));
			Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Bar.mStatusOverflowText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
