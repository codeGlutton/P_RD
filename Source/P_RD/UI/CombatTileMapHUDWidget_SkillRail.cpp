#include "UI/CombatTileMapHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/CombatTileMapHUDWidgetPrivate.h"
#include "UI/IndexedButtonWidget.h"

using namespace RDCombatHUD;

void UCombatTileMapHUDWidget::RebuildSkillRailWidgets()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr
		|| (mSkillRailPanels.Num() == CombatSkillSlotCount
			&& (IsDesignerSkinActive() == false || mSkillDrawerHandleButton != nullptr)))
	{
		return;
	}

	for (UBorder* SkillRailPanel : mSkillRailPanels)
	{
		if (SkillRailPanel != nullptr)
		{
			SkillRailPanel->RemoveFromParent();
		}
	}
	for (UImage* SkillRailIcon : mSkillRailIcons)
	{
		if (SkillRailIcon != nullptr)
		{
			SkillRailIcon->RemoveFromParent();
		}
	}
	for (UTextBlock* SkillRailText : mSkillRailTexts)
	{
		if (SkillRailText != nullptr)
		{
			SkillRailText->RemoveFromParent();
		}
	}
	mSkillRailPanels.Reset();
	mSkillRailIcons.Reset();
	mSkillRailTexts.Reset();
	if (mSkillDrawerHandleButton != nullptr)
	{
		mSkillDrawerHandleButton->RemoveFromParent();
		mSkillDrawerHandleButton = nullptr;
		mSkillDrawerHandleText = nullptr;
	}

	// 슬롯 6칸의 뼈대(패널/아이콘/라벨)만 만든다. 어떤 슬롯에 무엇이 보이는지는
	// RefreshSkillRailWidgets가 보유 스킬 스냅샷(FSkillUI)만으로 결정한다(시안 라벨 폴백 없음).
	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UBorder* SkillRailPanel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailPanel_%d"), SkillIndex))
		);
		UImage* SkillRailIcon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailIcon_%d"), SkillIndex))
		);
		UTextBlock* SkillRailText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("RuntimeSkillRailText_%d"), SkillIndex))
		);
		if (SkillRailPanel == nullptr || SkillRailIcon == nullptr || SkillRailText == nullptr)
		{
			continue;
		}

		SkillRailPanel->SetPadding(GetCombatSkillRailPadding());
		SkillRailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		SkillRailText->SetJustification(ETextJustify::Left);
		SkillRailText->SetClipping(EWidgetClipping::ClipToBounds);
		FSlateFontInfo SkillNameFont = SkillRailText->GetFont();
		// 360px 서랍 안에서 한글 7~8자 스킬명도 한 줄로 끝까지 보이게 한다.
		SkillNameFont.Size = 26;
		SkillNameFont.OutlineSettings.OutlineSize = 2;
		SkillNameFont.OutlineSettings.OutlineColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.85f);
		SkillRailText->SetFont(SkillNameFont);
		SkillRailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.98f, 0.92f, 1.0f)));
		SkillRailText->SetVisibility(ESlateVisibility::Collapsed);

		// 스킨 활성 시 DesignCanvas: 렌더/히트테스트가 같은 Safe Area 비례 좌표계를 쓴다.
		// RootCanvas(뷰포트)에 붙이면 디자인 스케일과 입력 영역이 서로 어긋난다.
		UCanvasPanel* SkillRailCanvas = GetSkinTargetCanvas();
		SkillRailCanvas->AddChildToCanvas(SkillRailPanel);
		SkillRailCanvas->AddChildToCanvas(SkillRailIcon);
		SkillRailCanvas->AddChildToCanvas(SkillRailText);

		mSkillRailPanels.Add(SkillRailPanel);
		mSkillRailIcons.Add(SkillRailIcon);
		mSkillRailTexts.Add(SkillRailText);
	}

	// 모바일 스킨에서만 아이콘 위치를 고정하고 이름 영역을 옆으로 여는 서랍 손잡이를 붙인다.
	if (IsDesignerSkinActive())
	{
		mSkillDrawerHandleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SkillDrawerHandleButton"));
		mSkillDrawerHandleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SkillDrawerHandleText"));
		if (mSkillDrawerHandleButton != nullptr && mSkillDrawerHandleText != nullptr)
		{
			mSkillDrawerHandleButton->SetBackgroundColor(FLinearColor(0.10f, 0.18f, 0.22f, 0.96f));
			mSkillDrawerHandleButton->SetTouchMethod(EButtonTouchMethod::PreciseTap);
			mSkillDrawerHandleButton->OnClicked.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillDrawerToggleClicked);

			FSlateFontInfo HandleFont = mSkillDrawerHandleText->GetFont();
			HandleFont.Size = 48;
			HandleFont.OutlineSettings.OutlineSize = 2;
			mSkillDrawerHandleText->SetFont(HandleFont);
			mSkillDrawerHandleText->SetJustification(ETextJustify::Center);
			mSkillDrawerHandleText->SetText(FText::FromString(TEXT(">")));
			mSkillDrawerHandleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.88f, 0.42f, 1.0f)));
			mSkillDrawerHandleButton->AddChild(mSkillDrawerHandleText);
			GetSkinTargetCanvas()->AddChildToCanvas(mSkillDrawerHandleButton);
		}
	}

	RefreshSkillRailWidgets();
	ApplyRuntimeWidgetLayout();
}

/** @details 레일 고정 배치 - 맨 위 칸=기본 공격(평타), 맨 아래 칸=STEP, 중간 4칸=추가 스킬(시각 슬롯 매핑).
 * 슬롯 2상태 - [보유] WBP 빈 프레임 안에 아이콘+코스트를 채운다(사용불가면 흐림) / [미보유] 아무것도 안 그린다.
 * WBP에는 빈 프레임(크롬)만 있으므로 "가리기(검은 커버)"가 필요 없다. 데이터 소스는 FSkillUI 단일. */
void UCombatTileMapHUDWidget::RefreshSkillRailWidgets()
{
	int32 SkillIndex = mCombatUIModel != nullptr ? mCombatUIModel->GetSelectedSkillIndex() : INDEX_NONE;
	if (mSelectedSkillIndex != SkillIndex)
	{
		ClearOwnedDiceSelectionHighlight();
	}
	mSelectedSkillIndex = SkillIndex;

	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;

	for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillRailPanels.Num(); ++RailSlotIndex)
	{
		const int32 SkillDataIndex = GetSkillDataIndexForRailSlot(RailSlotIndex);
		const FSkillUI* Skill = (Skills != nullptr && Skills->IsValidIndex(SkillDataIndex)) ? &(*Skills)[SkillDataIndex] : nullptr;
		const bool bOwned = Skill != nullptr && Skill->mName.IsEmpty() == false;
		const bool bUsable = bOwned && Skill->mIsUsable;
		const bool bSelected = bOwned && SkillDataIndex == mSelectedSkillIndex;
		const float DimOpacity = bUsable ? 1.0f : 0.45f;

		if (UBorder* SkillRailPanel = mSkillRailPanels[RailSlotIndex])
		{
			if (bOwned == false)
			{
				// 미보유: 슬롯을 아예 비운다 - WBP의 빈 프레임이 그대로 "빈 칸"으로 보인다.
				// 브러시도 투명화해 둔다(방어): 외부에서 visibility를 강제 복원해도 UBorder 기본 흰 브러시가 새지 않게.
				SkillRailPanel->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
				SkillRailPanel->SetVisibility(ESlateVisibility::Collapsed);
			}
			else
			{
				SkillRailPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				// 기존 96px WBP 프레임 대신 모바일 행 전체를 런타임 배경으로 그린다.
				SkillRailPanel->SetBrushColor(GetCombatSkillRailBrushColor(bSelected));
				SkillRailPanel->SetRenderScale(FVector2D::UnitVector);
			}
		}

		if (mSkillRailIcons.IsValidIndex(RailSlotIndex))
		{
			if (UImage* SkillRailIcon = mSkillRailIcons[RailSlotIndex])
			{
				if (bOwned && Skill->mIcon != nullptr)
				{
					SkillRailIcon->SetBrushFromTexture(Skill->mIcon, false);
					SkillRailIcon->SetRenderOpacity(DimOpacity);
					SkillRailIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					SkillRailIcon->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}

		if (mSkillRailTexts.IsValidIndex(RailSlotIndex))
		{
			if (UTextBlock* SkillRailText = mSkillRailTexts[RailSlotIndex])
			{
				SkillRailText->SetText(bOwned ? Skill->mName : FText::GetEmpty());
			}
		}
	}

	// 라벨 투명도/표시는 서랍 펼침 정도에 묶여 있어 애니 프레임과 공용 헬퍼로 뺐다.
	ApplySkillDrawerLabelVisuals();
}

/** @details 서랍 애니메이션 프레임마다 필요한 건 이것뿐이라 RefreshSkillRailWidgets(브러시/텍스트/선택 강조 전체)에서 분리했다. */
void UCombatTileMapHUDWidget::ApplySkillDrawerLabelVisuals() const
{
	const TArray<FSkillUI>* Skills = mCombatUIModel != nullptr ? &mCombatUIModel->GetSkillUIs() : nullptr;
	for (int32 RailSlotIndex = 0; RailSlotIndex < mSkillRailTexts.Num(); ++RailSlotIndex)
	{
		UTextBlock* SkillRailText = mSkillRailTexts[RailSlotIndex];
		if (SkillRailText == nullptr)
		{
			continue;
		}

		// GetSkillDataIndexForRailSlot이 보유 여부(FSkillUI 이름)까지 검증한다 — INDEX_NONE이면 미보유.
		const int32 SkillDataIndex = GetSkillDataIndexForRailSlot(RailSlotIndex);
		const bool bOwned = SkillDataIndex != INDEX_NONE;
		const bool bUsable = bOwned && Skills != nullptr && (*Skills)[SkillDataIndex].mIsUsable;
		const float DimOpacity = bUsable ? 1.0f : 0.45f;

		SkillRailText->SetRenderOpacity(DimOpacity * mSkillDrawerExpansion);
		SkillRailText->SetVisibility(
			bOwned && mSkillDrawerExpansion > KINDA_SMALL_NUMBER
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

void UCombatTileMapHUDWidget::HandleSkillDrawerToggleClicked()
{
	mSkillDrawerExpansionTarget = mSkillDrawerExpansionTarget > 0.5f ? 0.0f : 1.0f;
	if (mSkillDrawerHandleText != nullptr)
	{
		mSkillDrawerHandleText->SetText(FText::FromString(
			mSkillDrawerExpansionTarget > 0.5f ? TEXT("<") : TEXT(">")));
	}
}

void UCombatTileMapHUDWidget::UpdateSkillDrawerAnimation(float InDeltaTime)
{
	// 지도·결과 화면에서 레일을 숨긴 동안 애니메이션 refresh가 자식들을 다시 표시하지 않게 한다.
	if (mCombatControlsHidden)
	{
		return;
	}

	if (FMath::IsNearlyEqual(mSkillDrawerExpansion, mSkillDrawerExpansionTarget, 0.001f))
	{
		if (mSkillDrawerExpansion != mSkillDrawerExpansionTarget)
		{
			mSkillDrawerExpansion = mSkillDrawerExpansionTarget;
			ApplyRuntimeWidgetLayout();
			RefreshSkillRailWidgets();
		}
		return;
	}

	const float UnitsPerSecond = 1.0f / FMath::Max(MobileSkillDrawerAnimationSeconds, KINDA_SMALL_NUMBER);
	mSkillDrawerExpansion = FMath::FInterpConstantTo(
		mSkillDrawerExpansion, mSkillDrawerExpansionTarget, InDeltaTime, UnitsPerSecond);
	// 애니 프레임에는 서랍 폭에 의존하는 것만 갱신한다 — 전체 ApplyRuntimeWidgetLayout(HUD 위젯 전부 재배치)과
	// RefreshSkillRailWidgets(브러시/텍스트 재설정)를 매 프레임 돌리면 애니 내내 Layout 무효화가 발생한다.
	// 목표 도달 시(위 스냅 분기)에는 기존대로 전체 레이아웃/리프레시로 마무리한다.
	ApplySkillRailSlotLayout();
	ApplySkillDrawerLabelVisuals();
}

/** @details 시각 슬롯 규칙은 헤더 주석 참고. 반환 전에 보유 여부(FSkillUI 이름)까지 검증해 미보유면 INDEX_NONE. */
int32 UCombatTileMapHUDWidget::GetSkillDataIndexForRailSlot(int32 RailSlotIndex) const
{
	int32 SkillDataIndex = INDEX_NONE;
	if (RailSlotIndex == 0)
	{
		SkillDataIndex = 0;                                // 맨 위 고정: 기본 공격(평타)
	}
	else if (RailSlotIndex == CombatSkillSlotCount - 1)
	{
		SkillDataIndex = 1;                                // 맨 아래 고정: 기본 이동(STEP)
	}
	else if (RailSlotIndex >= 1 && RailSlotIndex < CombatSkillSlotCount - 1)
	{
		SkillDataIndex = RailSlotIndex + 1;                // 중간 4칸: 추가 스킬(데이터 2..5)을 위에서부터
	}

	if (mCombatUIModel == nullptr)
	{
		return INDEX_NONE;
	}
	const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
	if (Skills.IsValidIndex(SkillDataIndex) == false || Skills[SkillDataIndex].mName.IsEmpty() == true)
	{
		return INDEX_NONE;
	}
	return SkillDataIndex;
}

/** @details 시안 라벨 폴백 없이 뷰모델의 보유 스킬 이름만 반환한다(없으면 빈 텍스트). */
FText UCombatTileMapHUDWidget::GetOwnedSkillLabel(int32 SkillIndex) const
{
	if (mCombatUIModel != nullptr)
	{
		const TArray<FSkillUI>& Skills = mCombatUIModel->GetSkillUIs();
		if (Skills.IsValidIndex(SkillIndex))
		{
			return Skills[SkillIndex].mName;
		}
	}
	return FText::GetEmpty();
}

void UCombatTileMapHUDWidget::EnsureSkillInputButtons()
{
	if (RootCanvas == nullptr || WidgetTree == nullptr || mSkillInputButtons.Num() == CombatSkillSlotCount)
	{
		return;
	}

	for (UIndexedButtonWidget* SkillInputButton : mSkillInputButtons)
	{
		if (SkillInputButton != nullptr)
		{
			SkillInputButton->RemoveFromParent();
		}
	}
	mSkillInputButtons.Reset();

	for (int32 SkillIndex = 0; SkillIndex < CombatSkillSlotCount; ++SkillIndex)
	{
		UIndexedButtonWidget* SkillInputButton = WidgetTree->ConstructWidget<UIndexedButtonWidget>(
			UIndexedButtonWidget::StaticClass(),
			FName(*FString::Printf(TEXT("SkillInputButton_%d"), SkillIndex))
		);
		if (SkillInputButton == nullptr)
		{
			continue;
		}

		SkillInputButton->SetBackgroundColor(GetTransparentInputButtonColor());
		SkillInputButton->SetVisibility(ESlateVisibility::Visible);
		SkillInputButton->SetButtonIndex(SkillIndex);
		SkillInputButton->OnIndexedPressed.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonPressed);
		SkillInputButton->OnReleased.AddUniqueDynamic(this, &UCombatTileMapHUDWidget::HandleSkillButtonReleased);
		GetSkinTargetCanvas()->AddChildToCanvas(SkillInputButton);   // 렌더(패널)와 같은 좌표계 — 16:9 외 화면에서 탭 영역 어긋남 방지
		mSkillInputButtons.Add(SkillInputButton);
	}
}

int32 UCombatTileMapHUDWidget::FindSkillRailIndexAtScreenPosition(const FVector2D& ScreenPosition) const
{
	// 렌더된 입력 버튼 자신의 geometry로 판정한다 — 좌표계(엣지 피닝/레터박스/레거시)와 무관하게
	// 렌더와 히트테스트가 항상 일치한다. 첫 페인트 전(크기 0)이나 비표시는 건너뛴다.
	for (int32 SkillIndex = 0; SkillIndex < mSkillInputButtons.Num(); ++SkillIndex)
	{
		const UWidget* InputButton = mSkillInputButtons[SkillIndex];
		if (InputButton == nullptr || InputButton->IsVisible() == false)
		{
			continue;
		}
		const FGeometry& ButtonGeometry = InputButton->GetCachedGeometry();
		const FVector2D LocalSize = ButtonGeometry.GetLocalSize();
		if (LocalSize.X <= 0.0f || LocalSize.Y <= 0.0f)
		{
			continue;
		}
		if (ButtonGeometry.IsUnderLocation(ScreenPosition))
		{
			return SkillIndex;
		}
	}
	return INDEX_NONE;
}
