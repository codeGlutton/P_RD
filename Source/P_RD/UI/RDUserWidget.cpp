#include "UI/RDUserWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateTypes.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// 눌렀을 때 버튼이 제자리에서 살짝 줄어드는 비율(보조 효과).
	constexpr float ButtonPressScale = 0.94f;

	// 눌렀을 때 배경색(멀티플라이어) RGB에 곱하는 값 — 어둡게 해서 눈에 띄게 한다(주 효과).
	constexpr float ButtonPressColorMul = 0.6f;

	/** 한글 버튼 라벨의 fallback font 메트릭을 판별한다. */
	bool ContainsHangul(const FString& Value)
	{
		for (const TCHAR Character : Value)
		{
			const uint32 CodePoint = static_cast<uint32>(Character);
			if ((CodePoint >= 0xAC00 && CodePoint <= 0xD7A3)
				|| (CodePoint >= 0x1100 && CodePoint <= 0x11FF)
				|| (CodePoint >= 0x3130 && CodePoint <= 0x318F)
				|| (CodePoint >= 0xA960 && CodePoint <= 0xA97F)
				|| (CodePoint >= 0xD7B0 && CodePoint <= 0xD7FF))
			{
				return true;
			}
		}
		return false;
	}

	/** 글자가 클수록 커지는 보정량. 작은 HUD 라벨은 과하게 움직이지 않게 제한한다. */
	float KoreanOpticalCenterOffsetY(const UTextBlock& Text)
	{
		return -FMath::Clamp(static_cast<float>(Text.GetFont().Size) * 0.08f,
			1.0f, 4.0f);
	}

	void SplitProfiledButtonName(const FString& FullName, FString& OutBase,
		FString& OutSuffix)
	{
		OutBase = FullName;
		OutSuffix.Reset();
		const int32 ProfileSeparator = FullName.Find(
			TEXT("__"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
		if (ProfileSeparator != INDEX_NONE)
		{
			OutBase = FullName.Left(ProfileSeparator);
			OutSuffix = FullName.Mid(ProfileSeparator);
		}
	}
}

/**
 * @brief 위젯 생성 직후에는 공통 OpenUI() 요청 전까지 보이지 않게 둔다.
 */
URDUserWidget::URDUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);

	// 공용 버튼 클릭 사운드(SVN uasset)를 하드레퍼런스로 프리로드 → 쿡 보장(#300 컨벤션, 문자열 소프트로드 대신).
	static ConstructorHelpers::FObjectFinder<USoundBase> ButtonPressSoundFinder(TEXT("/Game/SVN/OutSideAsset/AICreation/Audio/UISFX/SFX_UI_Click_Scratch003.SFX_UI_Click_Scratch003"));
	if (ButtonPressSoundFinder.Succeeded())
	{
		mCommonButtonPressSound = ButtonPressSoundFinder.Object;
	}
}

/**
 * @brief 뷰포트 등록과 열기 연출을 한 경로로 묶는다.
 *
 * @details
 * 호출자는 HUD인지 팝업인지, 이미 뷰포트에 있는지, 애니메이션이 있는지 몰라도 OpenUI()만 호출하면 된다.
 * 이미 열린 상태에서 다시 호출된 경우에도 후속 로직은 이어져야 하므로 콜백은 즉시 실행한다.
 */
void URDUserWidget::OpenUI(FOnEndUIOpenAnimation Callback)
{
	if (IsOpened())
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	OnEndUIOpenAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Opening;
	ApplyOpenUI();
	PlayOpenUIAnimation();
}

/**
 * @brief 닫기 연출과 실제 닫힘 처리를 한 경로로 묶는다.
 *
 * @details
 * 닫힌 위젯을 닫는 요청은 성공한 닫힘으로 취급해 콜백을 실행한다.
 * 반대로 닫히는 중인 위젯에 대한 중복 요청은 같은 애니메이션과 콜백이 다시 쌓이지 않도록 무시한다.
 */
void URDUserWidget::CloseUI(FOnEndUICloseAnimation Callback)
{
	if (mLifecycleState == ERDUserWidgetLifecycleState::Closed)
	{
		if (Callback.IsBound())
		{
			Callback.Execute(this);
		}
		return;
	}

	if (mLifecycleState == ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	OnEndUICloseAnimation = MoveTemp(Callback);
	mLifecycleState = ERDUserWidgetLifecycleState::Closing;
	PlayCloseUIAnimation();
}

/**
 * @brief 내부 생명주기와 실제 UMG 표시 상태가 모두 열림 조건을 만족하는지 확인한다.
 */
bool URDUserWidget::IsOpened() const
{
	return (mLifecycleState == ERDUserWidgetLifecycleState::Opening
			|| mLifecycleState == ERDUserWidgetLifecycleState::Open)
		&& IsInViewport()
		&& IsVisible();
}

/**
 * @brief Blueprint 또는 기본 구현에서 열기 연출 완료 시점을 확정한다.
 */
void URDUserWidget::FinishOpenUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Opening)
	{
		return;
	}

	mLifecycleState = ERDUserWidgetLifecycleState::Open;
	if (OnEndUIOpenAnimation.IsBound())
	{
		OnEndUIOpenAnimation.Execute(this);
		OnEndUIOpenAnimation.Unbind();
	}
}

/**
 * @brief Blueprint 또는 기본 구현에서 닫기 연출 완료 시점을 확정한다.
 */
void URDUserWidget::FinishCloseUI()
{
	if (mLifecycleState != ERDUserWidgetLifecycleState::Closing)
	{
		return;
	}

	ApplyCloseUI();
	mLifecycleState = ERDUserWidgetLifecycleState::Closed;
	if (OnEndUICloseAnimation.IsBound())
	{
		OnEndUICloseAnimation.Execute(this);
		OnEndUICloseAnimation.Unbind();
	}
}

/**
 * @brief 별도 Blueprint 연출이 없으면 즉시 열린 상태로 완료한다.
 */
void URDUserWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

/**
 * @brief 별도 Blueprint 연출이 없으면 즉시 닫힌 상태로 완료한다.
 */
void URDUserWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}

/**
 * @brief 공통 OpenUI()가 선택한 ZOrder로 뷰포트에 올리고 Visible 상태를 보장한다.
 */
void URDUserWidget::ApplyOpenUI()
{
	if (IsInViewport() == false)
	{
		AddToViewport(GetViewportZOrder());
	}

	SetVisibility(ESlateVisibility::Visible);
}

/**
 * @brief 위젯별 닫힘 정책에 따라 제거하거나 숨김 상태로 보존한다.
 */
void URDUserWidget::ApplyCloseUI()
{
	if (ShouldRemoveFromParentOnClose())
	{
		RemoveFromParent();
		return;
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

int32 URDUserWidget::GetViewportZOrder() const
{
	return mViewportZOrder;
}

bool URDUserWidget::ShouldRemoveFromParentOnClose() const
{
	return mRemoveFromParentOnClose;
}

bool URDUserWidget::ShouldApplyButtonFeedback() const
{
	// 기본은 미적용. 타이틀/클래스 선택 등 프론트엔드 화면만 override로 켠다(전투 HUD에는 걸지 않는다).
	return false;
}

void URDUserWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	NormalizeCommonInputLayersAndButtonLabels();
	// 클릭 사운드는 모든 화면의 모든 버튼에 공통 적용한다. 시각 피드백(어둡게/축소)만 화면별 opt-in.
	SetupCommonButtonFeedback();
}

void URDUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// WBP에 구워진 버튼은 NativeOnInitialized에서 처리된다. 다만 파생 클래스는
	// Super::NativeConstruct() 뒤에 ConstructWidget으로 버튼을 만드는 경우가 있다.
	// 현재 호출과 다음 틱 재검사를 함께 두면 두 종류 모두 같은 소리를 쓴다.
	NormalizeCommonInputLayersAndButtonLabels();
	SetupCommonButtonFeedback();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					NormalizeCommonInputLayersAndButtonLabels();
					SetupCommonButtonFeedback();
				}));
	}
}

void URDUserWidget::NormalizeCommonInputLayersAndButtonLabels()
{
	if (WidgetTree == nullptr)
	{
		return;
	}

	// 그림과 글자는 시각 요소다. Visible 상태로 버튼 위에 놓이면 Slate 히트
	// 경로의 앞쪽을 차지할 수 있으므로 자신과 자식 모두 입력 대상에서 뺀다.
	// 순수 배치 패널은 자신만 빼고 자식 버튼은 계속 입력을 받게 한다.
	WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (Widget == nullptr)
			{
				return;
			}
			if ((Widget->IsA<UImage>() || Widget->IsA<UTextBlock>())
				&& (Widget->GetVisibility() == ESlateVisibility::Visible
					|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible))
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		});

	TArray<UButton*> Buttons;
	WidgetTree->ForEachWidget([&Buttons](UWidget* Widget)
		{
			if (UButton* Button = Cast<UButton>(Widget))
			{
				Buttons.Add(Button);
			}
		});

	for (UButton* Button : Buttons)
	{
		TArray<UWidget*> Companions;
		CollectButtonCompanions(Button, Companions);
		for (UWidget* Companion : Companions)
		{
			UTextBlock* Text = Cast<UTextBlock>(Companion);
			if (Text == nullptr)
			{
				continue;
			}

			NormalizeCommonButtonLabel(Text);
		}
	}
}

void URDUserWidget::NormalizeCommonButtonLabel(UTextBlock* Text)
{
	if (Text == nullptr)
	{
		return;
	}

	Text->SetJustification(ETextJustify::Center);
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Text->Slot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Center);
	}
	else if (UScaleBoxSlot* ScaleSlot = Cast<UScaleBoxSlot>(Text->Slot))
	{
		ScaleSlot->SetHorizontalAlignment(HAlign_Center);
		ScaleSlot->SetVerticalAlignment(VAlign_Center);
	}

	const TWeakObjectPtr<UTextBlock> Key(Text);
	const FVector2D CurrentTranslation = Text->GetRenderTransform().Translation;
	FVector2D* BaseTranslation = mButtonLabelBaseTranslations.Find(Key);
	const FVector2D* LastApplied = mButtonLabelLastAppliedTranslations.Find(Key);
	if (BaseTranslation == nullptr)
	{
		mButtonLabelBaseTranslations.Add(Key, CurrentTranslation);
		BaseTranslation = mButtonLabelBaseTranslations.Find(Key);
	}
	else if (LastApplied != nullptr && !CurrentTranslation.Equals(*LastApplied, KINDA_SMALL_NUMBER))
	{
		// 파생 화면이 Super::NativeConstruct() 이후 별도 배치를 적용했다면
		// 그 값을 새 기준으로 받아들여 화면 고유 보정을 덮어쓰지 않는다.
		*BaseTranslation = CurrentTranslation;
	}

	const float OpticalOffsetY = ContainsHangul(Text->GetText().ToString())
		? KoreanOpticalCenterOffsetY(*Text) : 0.0f;
	const FVector2D DesiredTranslation = *BaseTranslation
		+ FVector2D(0.0f, OpticalOffsetY);
	FWidgetTransform Transform = Text->GetRenderTransform();
	Transform.Translation = DesiredTranslation;
	Text->SetRenderTransform(Transform);
	mButtonLabelLastAppliedTranslations.Add(Key, DesiredTranslation);
}

void URDUserWidget::ApplyCommonButtonPressSound(UButton* Button) const
{
	if (Button == nullptr || mCommonButtonPressSound == nullptr
		|| Button->GetName().Contains(TEXT("InputBlocker")))
	{
		return;
	}

	FButtonStyle Style = Button->GetStyle();
	Style.PressedSlateSound.SetResourceObject(mCommonButtonPressSound);
	Button->SetStyle(Style);
}

void URDUserWidget::SetupCommonButtonFeedback()
{
	mFeedbackButtons.Reset();
	mFeedbackButtonBaseColors.Reset();
	mFeedbackButtonBaseTransforms.Reset();
	mFeedbackButtonBasePivots.Reset();

	if (WidgetTree == nullptr)
	{
		return;
	}

	const bool bApplyPressVisual = ShouldApplyButtonFeedback();

	// 이 위젯 트리 안의 모든 UButton을 순회한다(자식 UserWidget은 각자 이 베이스를 상속하므로 스스로 처리).
	WidgetTree->ForEachWidget([this, bApplyPressVisual](UWidget* Widget)
		{
			UButton* Button = Cast<UButton>(Widget);
			if (Button == nullptr)
			{
				return;
			}

			// 클릭 사운드: 스타일의 PressedSlateSound에만 주입한다. 브러시/색은 그대로 둬 디자이너 스킨을 보존한다.
			{
				ApplyCommonButtonPressSound(Button);
				FButtonStyle Style = Button->GetStyle();

				/*
				 * 호버 피드백(0811 점검). 마우스를 올려도 아무 변화가 없어
				 * "반응이 없다" 로 읽혔다 -- 모든 화면 공통으로, 호버 브러시가
				 * 비어 있는(투명) 버튼에만 옅은 밝김을 얹는다. 디자이너가 호버
				 * 그림을 그려 둔 버튼은 그대로 두고, 터치(모바일)에는 호버가
				 * 없으므로 자연히 무효과다.
				 */
				const FSlateBrush& Hovered = Style.Hovered;
				const bool bHoverInvisible =
					Hovered.DrawAs == ESlateBrushDrawType::NoDrawType
					|| (Hovered.GetResourceObject() == nullptr
						&& Hovered.TintColor.GetSpecifiedColor().A <= 0.01f);
				if (bHoverInvisible)
				{
					FSlateBrush HoverBrush;
					HoverBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.12f));
					Style.SetHovered(HoverBrush);
				}
				Button->SetStyle(Style);
			}

			// 누름 시각 피드백은 opt-in 화면(타이틀/클래스 선택 등)만 — 전투 HUD에는 걸지 않는다.
			if (bApplyPressVisual == false)
			{
				return;
			}

			// 누름 피드백: 배경색을 어둡게(주 효과) + 살짝 축소(보조). 브러시 자체는 안 건드려 디자이너 스킨 보존.
			// 원래 배경색을 저장해 뒀다가 떼면 그대로 복원한다(버튼별 커스텀 틴트도 안전).
			Button->OnPressed.AddUniqueDynamic(this, &URDUserWidget::HandleAnyButtonPressed);
			Button->OnReleased.AddUniqueDynamic(this, &URDUserWidget::HandleAnyButtonReleased);

			mFeedbackButtons.Add(Button);
			mFeedbackButtonBaseColors.Add(Button->GetBackgroundColor());
			mFeedbackButtonBaseTransforms.Add(Button->GetRenderTransform());
			mFeedbackButtonBasePivots.Add(Button->GetRenderTransformPivot());
		});
}

void URDUserWidget::CollectButtonCompanions(const UButton* Button, TArray<UWidget*>& OutCompanions) const
{
	if (Button == nullptr || WidgetTree == nullptr)
	{
		return;
	}

	// 버튼명을 "<Base>__<Profile>"로 보고 Base/접미사를 분리한다.
	FString Base;
	FString Suffix;
	SplitProfiledButtonName(Button->GetName(), Base, Suffix);
	const TSet<FString> ExactCompanionNames =
	{
		Base + TEXT("Text") + Suffix,
		Base + TEXT("Label") + Suffix,
		Base + TEXT("FrameImage") + Suffix,
		Base + TEXT("Plate") + Suffix,
		Base + TEXT("Art") + Suffix,
	};

	// 비용·상태·설명처럼 같은 prefix를 공유하는 요소는 건드리지 않는다.
	// 공통 처리는 정확한 라벨/프레임 이름만 맡고, 특수 화면은 builder가 명시적으로 정렬한다.
	WidgetTree->ForEachWidget([&ExactCompanionNames, Button, &OutCompanions](UWidget* Widget)
		{
			if (Widget == nullptr || Widget == Button)
			{
				return;
			}
			if (Widget->IsA<UImage>() == false && Widget->IsA<UTextBlock>() == false)
			{
				return;
			}
			if (ExactCompanionNames.Contains(Widget->GetName()))
			{
				OutCompanions.Add(Widget);
			}
		});
}

void URDUserWidget::HandleAnyButtonPressed()
{
	// OnPressed는 발신 버튼을 알려주지 않으므로, 실제로 눌린 버튼만 골라 처리한다(동시에 하나만 눌리는 게 보통).
	for (int32 Index = 0; Index < mFeedbackButtons.Num(); ++Index)
	{
		UButton* Button = mFeedbackButtons[Index];
		if (Button == nullptr || Button->IsPressed() == false)
		{
			continue;
		}

		// (1) 버튼 자체 — 버튼 브러시가 시각요소인 경우(클래스 선택 등)에 배경색을 어둡게 + 축소.
		FLinearColor Pressed = mFeedbackButtonBaseColors.IsValidIndex(Index) ? mFeedbackButtonBaseColors[Index] : FLinearColor::White;
		Pressed.R *= ButtonPressColorMul;
		Pressed.G *= ButtonPressColorMul;
		Pressed.B *= ButtonPressColorMul;   // 알파는 유지, RGB만 어둡게.
		Button->SetBackgroundColor(Pressed);
		const FWidgetTransform BaseTransform = mFeedbackButtonBaseTransforms.IsValidIndex(Index)
			? mFeedbackButtonBaseTransforms[Index] : Button->GetRenderTransform();
		FWidgetTransform PressedTransform = BaseTransform;
		PressedTransform.Scale *= ButtonPressScale;
		Button->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		Button->SetRenderTransform(PressedTransform);

		// (2) 동반 시각 위젯 — 투명 버튼 위에 얹힌 프레임/텍스트(타이틀 등). 이쪽을 어둡게+축소해야 실제로 보인다.
		//     이전 잔여가 있으면 먼저 복원 후 새로 적용(한 번에 한 버튼만 눌린다는 전제).
		RestoreActivePressCompanions();
		TArray<UWidget*> Companions;
		CollectButtonCompanions(Button, Companions);
		for (UWidget* Companion : Companions)
		{
			const FWidgetTransform CompanionBaseTransform = Companion->GetRenderTransform();
			const FVector2D CompanionBasePivot = Companion->GetRenderTransformPivot();
			Companion->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
			FWidgetTransform CompanionPressedTransform = CompanionBaseTransform;
			CompanionPressedTransform.Scale *= ButtonPressScale;
			Companion->SetRenderTransform(CompanionPressedTransform);

			FLinearColor CompanionBase = FLinearColor::White;
			if (UImage* Image = Cast<UImage>(Companion))
			{
				CompanionBase = Image->GetColorAndOpacity();
				FLinearColor Dark = CompanionBase;
				Dark.R *= ButtonPressColorMul;
				Dark.G *= ButtonPressColorMul;
				Dark.B *= ButtonPressColorMul;
				Image->SetColorAndOpacity(Dark);
			}
			else if (UTextBlock* Text = Cast<UTextBlock>(Companion))
			{
				CompanionBase = Text->GetColorAndOpacity().GetSpecifiedColor();
				FLinearColor Dark = CompanionBase;
				Dark.R *= ButtonPressColorMul;
				Dark.G *= ButtonPressColorMul;
				Dark.B *= ButtonPressColorMul;
				Text->SetColorAndOpacity(FSlateColor(Dark));
			}

			mActivePressCompanions.Add(Companion);
			mActivePressCompanionBaseColors.Add(CompanionBase);
			mActivePressCompanionBaseTransforms.Add(CompanionBaseTransform);
			mActivePressCompanionBasePivots.Add(CompanionBasePivot);
		}

		break;   // 한 번에 한 버튼만.
	}
}

void URDUserWidget::HandleAnyButtonReleased()
{
	// 버튼 자체 복원.
	for (int32 Index = 0; Index < mFeedbackButtons.Num(); ++Index)
	{
		UButton* Button = mFeedbackButtons[Index];
		if (Button == nullptr)
		{
			continue;
		}

		const FLinearColor Base = mFeedbackButtonBaseColors.IsValidIndex(Index) ? mFeedbackButtonBaseColors[Index] : FLinearColor::White;
		Button->SetBackgroundColor(Base);
		if (mFeedbackButtonBaseTransforms.IsValidIndex(Index))
		{
			Button->SetRenderTransform(mFeedbackButtonBaseTransforms[Index]);
		}
		if (mFeedbackButtonBasePivots.IsValidIndex(Index))
		{
			Button->SetRenderTransformPivot(mFeedbackButtonBasePivots[Index]);
		}
	}

	// 동반 위젯 복원.
	RestoreActivePressCompanions();
}

void URDUserWidget::RestoreActivePressCompanions()
{
	for (int32 Index = 0; Index < mActivePressCompanions.Num(); ++Index)
	{
		UWidget* Companion = mActivePressCompanions[Index].Get();
		if (Companion == nullptr)
		{
			continue;
		}

		const FLinearColor Base = mActivePressCompanionBaseColors.IsValidIndex(Index) ? mActivePressCompanionBaseColors[Index] : FLinearColor::White;
		if (UImage* Image = Cast<UImage>(Companion))
		{
			Image->SetColorAndOpacity(Base);
		}
		else if (UTextBlock* Text = Cast<UTextBlock>(Companion))
		{
			Text->SetColorAndOpacity(FSlateColor(Base));
		}
		if (mActivePressCompanionBaseTransforms.IsValidIndex(Index))
		{
			Companion->SetRenderTransform(mActivePressCompanionBaseTransforms[Index]);
		}
		if (mActivePressCompanionBasePivots.IsValidIndex(Index))
		{
			Companion->SetRenderTransformPivot(mActivePressCompanionBasePivots[Index]);
		}
	}

	mActivePressCompanions.Reset();
	mActivePressCompanionBaseColors.Reset();
	mActivePressCompanionBaseTransforms.Reset();
	mActivePressCompanionBasePivots.Reset();
}
