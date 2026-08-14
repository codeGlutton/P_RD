#include "UI/Combat/SkillCutInWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

namespace
{
	constexpr float EnterEnd = 0.20f;
	constexpr float SettleEnd = 0.34f;
	constexpr float ExitStart = 0.70f;
	constexpr float CutInAspectRatio = 1672.0f / 941.0f;
	constexpr float CutInViewportWidthFraction = 0.50f;
	constexpr float CutInViewportMaxHeightFraction = 0.50f;

	float Smooth01(const float Value)
	{
		const float T = FMath::Clamp(Value, 0.0f, 1.0f);
		return T * T * (3.0f - 2.0f * T);
	}

	float EaseOutCubic(const float Value)
	{
		const float T = FMath::Clamp(Value, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - T, 3.0f);
	}

	float TrianglePulse(const float Value, const float Center, const float HalfWidth)
	{
		return FMath::Clamp(1.0f - FMath::Abs(Value - Center) / HalfWidth, 0.0f, 1.0f);
	}

	void FillCanvas(UCanvasPanelSlot* Slot, const int32 ZOrder)
	{
		if (Slot == nullptr)
		{
			return;
		}
		Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetAutoSize(false);
		Slot->SetZOrder(ZOrder);
	}
}

USkillCutInWidget::USkillCutInWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

bool USkillCutInWidget::Initialize()
{
	const bool bWidgetInitialized = Super::Initialize();
	EnsureNativeWidgetTree();
	return bWidgetInitialized;
}

void USkillCutInWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureNativeWidgetTree();
}

void USkillCutInWidget::EnsureNativeWidgetTree()
{
	if (WidgetTree == nullptr || RootCanvas != nullptr)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("SkillCutInRoot"));
	WidgetTree->RootWidget = RootCanvas;

	DimLayer = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillCutInDim"));
	DimLayer->SetBrushColor(FLinearColor(0.005f, 0.0f, 0.012f, 0.72f));
	DimLayer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	FillCanvas(RootCanvas->AddChildToCanvas(DimLayer), 0);

	FixedBackgroundCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("SkillCutInFixedBackground"));
	FixedBackgroundCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	if (UCanvasPanelSlot* FixedBackgroundSlot = RootCanvas->AddChildToCanvas(FixedBackgroundCanvas))
	{
		FixedBackgroundSlot->SetAnchors(FAnchors(0.50f, 0.25f, 1.00f, 0.75f));
		FixedBackgroundSlot->SetAlignment(FVector2D::ZeroVector);
		FixedBackgroundSlot->SetOffsets(FMargin(0.0f));
		FixedBackgroundSlot->SetAutoSize(false);
		FixedBackgroundSlot->SetZOrder(9);
	}

	PanelCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("SkillCutInPanel"));
	PanelCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	PanelCanvas->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelCanvas))
	{
		// Bootstrap placement only. ApplyPresentation/ApplyMotion replace this
		// with a DPI-aware 1672:941 absolute layout for the active viewport.
		PanelSlot->SetAnchors(FAnchors(0.50f, 0.25f, 1.00f, 0.75f));
		PanelSlot->SetAlignment(FVector2D::ZeroVector);
		PanelSlot->SetOffsets(FMargin(0.0f));
		PanelSlot->SetAutoSize(false);
		PanelSlot->SetZOrder(10);
	}

	FixedFrontFXCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("SkillCutInFixedFrontFX"));
	FixedFrontFXCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	if (UCanvasPanelSlot* FixedFrontSlot = RootCanvas->AddChildToCanvas(FixedFrontFXCanvas))
	{
		FixedFrontSlot->SetAnchors(FAnchors(0.50f, 0.25f, 1.00f, 0.75f));
		FixedFrontSlot->SetAlignment(FVector2D::ZeroVector);
		FixedFrontSlot->SetOffsets(FMargin(0.0f));
		FixedFrontSlot->SetAutoSize(false);
		FixedFrontSlot->SetZOrder(11);
	}

	PanelFallback = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillCutInPanelFallback"));
	PanelFallback->SetBrushColor(FLinearColor(0.055f, 0.003f, 0.075f, 1.0f));
	FillCanvas(FixedBackgroundCanvas->AddChildToCanvas(PanelFallback), 0);

	AccentWedgeBack = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillCutInAccentBack"));
	AccentWedgeBack->SetBrushColor(FLinearColor(0.45f, 0.01f, 0.16f, 0.68f));
	AccentWedgeBack->SetRenderTransformAngle(-11.0f);
	AccentWedgeBack->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FillCanvas(FixedBackgroundCanvas->AddChildToCanvas(AccentWedgeBack), 1);

	BackgroundLayer = MakeImageLayer(FixedBackgroundCanvas, TEXT("SkillCutInBackground"), 2);
	SpeedLinesLayer = MakeImageLayer(FixedBackgroundCanvas, TEXT("SkillCutInSpeedBack"), 3);
	for (int32 LineIndex = 0; LineIndex < 7; ++LineIndex)
	{
		UBorder* SpeedLine = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), *FString::Printf(TEXT("SkillCutInSingleSpeedLine%d"), LineIndex));
		SpeedLine->SetBrushColor(FLinearColor(
			0.08f, 0.62f + 0.04f * (LineIndex % 3), 1.0f,
			LineIndex % 2 == 0 ? 0.52f : 0.32f));
		SpeedLine->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		FillCanvas(FixedBackgroundCanvas->AddChildToCanvas(SpeedLine), 3);
		SpeedLine->SetRenderScale(FVector2D(1.25f, 0.006f + 0.002f * (LineIndex % 3)));
		SpeedLine->SetRenderTransformAngle(-7.0f);
		SingleSpeedLineLayers.Add(SpeedLine);
	}
	SingleGoldWedgeLayer = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillCutInSingleGoldWedge"));
	SingleGoldWedgeLayer->SetBrushColor(FLinearColor(1.0f, 0.58f, 0.06f, 0.88f));
	SingleGoldWedgeLayer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FillCanvas(FixedBackgroundCanvas->AddChildToCanvas(SingleGoldWedgeLayer), 4);
	SingleGoldWedgeLayer->SetRenderScale(FVector2D(0.28f, 1.35f));
	SingleGoldWedgeLayer->SetRenderTransformAngle(-12.0f);
	RimLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInRim"), 4);
	MercenaryCapeLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenaryCape"), 5);
	MercenarySwordArmLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenarySwordArm"), 6);
	MercenaryShieldArmLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenaryShieldArm"), 7);
	SingleGhostCyanLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInSingleGhostCyan"), 6);
	SingleGhostGoldLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInSingleGhostGold"), 7);
	BodyLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInBody"), 8);
	MercenaryHeadLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenaryHead"), 9);
	MercenaryShieldLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenaryShield"), 10);
	EyeSocketLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInEyeSocket"), 11);
	EyeWhiteLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInEyeWhite"), 12);
	IrisLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInIris"), 13);
	PupilLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInPupil"), 14);
	MercenarySwordArcLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenarySwordArc"), 15);
	MercenaryImpactLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInMercenaryImpact"), 16);
	SingleFaceCropLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInSingleFaceCrop"), 17);
	SingleWeaponCropLayer = MakeImageLayer(PanelCanvas, TEXT("SkillCutInSingleWeaponCrop"), 18);
	ForegroundLayer = MakeImageLayer(FixedFrontFXCanvas, TEXT("SkillCutInForeground"), 19);
	FrameLayer = MakeImageLayer(FixedFrontFXCanvas, TEXT("SkillCutInFrame"), 20);
	FlashLayer = MakeImageLayer(FixedFrontFXCanvas, TEXT("SkillCutInFlash"), 21);

	AccentWedgeFront = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("SkillCutInAccentFront"));
	AccentWedgeFront->SetBrushColor(FLinearColor(1.0f, 0.08f, 0.34f, 0.36f));
	AccentWedgeFront->SetRenderTransformAngle(13.0f);
	AccentWedgeFront->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FillCanvas(FixedFrontFXCanvas->AddChildToCanvas(AccentWedgeFront), 22);
	AccentWedgeFront->SetRenderOpacity(0.0f);

	ResetLayerTransforms();
}

UImage* USkillCutInWidget::MakeImageLayer(
	UCanvasPanel* Parent,
	const FName Name,
	const int32 ZOrder)
{
	if (WidgetTree == nullptr || Parent == nullptr)
	{
		return nullptr;
	}

	UImage* Layer = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
	Layer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Layer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	FillCanvas(Parent->AddChildToCanvas(Layer), ZOrder);
	return Layer;
}

void USkillCutInWidget::SetLayerTexture(
	UImage* Layer,
	const TSoftObjectPtr<UTexture2D>& Texture,
	const FLinearColor& MissingTextureColor)
{
	if (Layer == nullptr)
	{
		return;
	}

	if (UTexture2D* LoadedTexture = Texture.LoadSynchronous())
	{
		Layer->SetBrushFromTexture(LoadedTexture, false);
		Layer->SetColorAndOpacity(FLinearColor::White);
		Layer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		return;
	}

	Layer->SetBrushFromTexture(nullptr, false);
	Layer->SetColorAndOpacity(MissingTextureColor);
	Layer->SetVisibility(MissingTextureColor.A > 0.001f
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::Collapsed);
}

void USkillCutInWidget::SetLayerUVRegion(
	UImage* Layer,
	const FVector2f& Minimum,
	const FVector2f& Maximum)
{
	if (Layer == nullptr)
	{
		return;
	}

	FSlateBrush Brush = Layer->GetBrush();
	Brush.SetUVRegion(FBox2f(Minimum, Maximum));
	Layer->SetBrush(Brush);
}

void USkillCutInWidget::ApplyPresentation(const FSkillCutInPresentationData& Presentation)
{
	const bool bSingleImageRig = Presentation.LayerRig == ESkillCutInLayerRig::MasterDuelSingle;
	const float ViewportScale = FMath::Max(
		KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
	UpdatePanelLayoutForViewport(UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale);
	SetLayerTexture(BackgroundLayer, Presentation.BackgroundTexture,
		bSingleImageRig ? FLinearColor::Transparent : FLinearColor(0.04f, 0.0f, 0.07f, 1.0f));
	SetLayerTexture(SpeedLinesLayer, Presentation.SpeedLinesTexture,
		FLinearColor::Transparent);
	for (UBorder* SpeedLine : SingleSpeedLineLayers)
	{
		if (SpeedLine != nullptr)
		{
			// V2 deliberately uses the generated background instead of seven extra
			// code streaks; keep these legacy prototype widgets out of the composition.
			SpeedLine->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	SingleGoldWedgeLayer->SetVisibility(ESlateVisibility::Collapsed);
	SetLayerTexture(RimLayer, Presentation.RimTexture, FLinearColor::Transparent);
	SetLayerTexture(MercenaryCapeLayer, Presentation.MercenaryCapeTexture,
		FLinearColor::Transparent);
	SetLayerTexture(MercenarySwordArmLayer, Presentation.MercenarySwordArmTexture,
		FLinearColor::Transparent);
	SetLayerTexture(MercenaryShieldArmLayer, Presentation.MercenaryShieldArmTexture,
		FLinearColor::Transparent);
	SetLayerTexture(SingleGhostCyanLayer,
		bSingleImageRig ? Presentation.BodyTexture : TSoftObjectPtr<UTexture2D>(),
		FLinearColor::Transparent);
	SetLayerTexture(SingleGhostGoldLayer,
		bSingleImageRig ? Presentation.BodyTexture : TSoftObjectPtr<UTexture2D>(),
		FLinearColor::Transparent);
	SetLayerTexture(BodyLayer, Presentation.BodyTexture,
		Presentation.AccentColor.CopyWithNewOpacity(0.9f));
	SetLayerTexture(MercenaryHeadLayer, Presentation.MercenaryHeadTexture,
		FLinearColor::Transparent);
	SetLayerTexture(MercenaryShieldLayer, Presentation.MercenaryShieldTexture,
		FLinearColor::Transparent);
	SetLayerTexture(EyeSocketLayer, Presentation.EyeSocketTexture,
		FLinearColor::Transparent);
	SetLayerTexture(EyeWhiteLayer, Presentation.EyeWhiteTexture,
		FLinearColor::Transparent);
	SetLayerTexture(IrisLayer, Presentation.IrisTexture, FLinearColor::Transparent);
	SetLayerTexture(PupilLayer, Presentation.PupilTexture, FLinearColor::Transparent);
	SetLayerTexture(MercenarySwordArcLayer, Presentation.MercenarySwordArcTexture,
		FLinearColor::Transparent);
	SetLayerTexture(MercenaryImpactLayer, Presentation.MercenaryImpactTexture,
		FLinearColor::Transparent);
	SetLayerTexture(SingleFaceCropLayer,
		bSingleImageRig ? Presentation.BodyTexture : TSoftObjectPtr<UTexture2D>(),
		FLinearColor::Transparent);
	SetLayerTexture(SingleWeaponCropLayer,
		bSingleImageRig ? Presentation.BodyTexture : TSoftObjectPtr<UTexture2D>(),
		FLinearColor::Transparent);
	SetLayerUVRegion(SingleGhostCyanLayer, FVector2f(0.0f), FVector2f(1.0f));
	SetLayerUVRegion(SingleGhostGoldLayer, FVector2f(0.0f), FVector2f(1.0f));
	SetLayerUVRegion(BodyLayer, FVector2f(0.0f), FVector2f(1.0f));
	// One short virtual close-up reuses the same imported plate. Each faction has
	// a crop tuned to its generated subject, without introducing another asset.
	SetLayerUVRegion(SingleFaceCropLayer,
		Presentation.bMirror ? FVector2f(0.30f, 0.02f) : FVector2f(0.27f, 0.12f),
		Presentation.bMirror ? FVector2f(0.64f, 0.56f) : FVector2f(0.58f, 0.76f));
	SetLayerUVRegion(SingleWeaponCropLayer, FVector2f(0.10f, 0.00f), FVector2f(0.46f, 0.53f));
	SetLayerTexture(ForegroundLayer, Presentation.ForegroundTexture,
		FLinearColor::Transparent);
	SetLayerTexture(FrameLayer, Presentation.FrameTexture, FLinearColor::Transparent);
	SetLayerTexture(FlashLayer, Presentation.FlashTexture, FLinearColor::Transparent);

	PanelFallback->SetBrushColor(bSingleImageRig
		? FLinearColor::Transparent
		: Presentation.AccentColor * FLinearColor(0.10f, 0.05f, 0.13f, 1.0f));
	AccentWedgeBack->SetBrushColor(bSingleImageRig
		? FLinearColor(0.02f, 0.22f, 0.65f, 0.62f)
		: Presentation.AccentColor.CopyWithNewOpacity(0.50f));
	AccentWedgeFront->SetBrushColor(bSingleImageRig
		? FLinearColor(1.0f, 0.58f, 0.06f, 0.72f)
		: Presentation.AccentColor.CopyWithNewOpacity(0.40f));
	ResetLayerTransforms();
}

bool USkillCutInWidget::PlayCutIn(
	const FSkillCutInPresentationData& Presentation,
	FOnSkillCutInFinished OnFinished)
{
	EnsureNativeWidgetTree();
	if (RootCanvas == nullptr || PanelCanvas == nullptr)
	{
		return false;
	}

	StopCutIn(false);
	ActivePresentation = Presentation;
	ActivePresentation.PupilAim.X = FMath::Clamp(ActivePresentation.PupilAim.X, -1.0f, 1.0f);
	ActivePresentation.PupilAim.Y = FMath::Clamp(ActivePresentation.PupilAim.Y, -1.0f, 1.0f);
	ActiveDurationSeconds = FMath::Max(0.10f, Presentation.DurationSeconds);
	ActiveFailSafeSeconds = FMath::Max(ActiveDurationSeconds, Presentation.FailSafeSeconds);
	FinishedCallback = MoveTemp(OnFinished);
	ElapsedSeconds = 0.0f;
	StartedAtRealTimeSeconds = FPlatformTime::Seconds();
	bCompletionDispatched = false;
	bCutInPlaying = true;
	ApplyPresentation(ActivePresentation);
	// The short presentation is modal: consume pointer input so commands and
	// world clicks cannot slip through while the pre-skill barrier is held.
	SetVisibility(ESlateVisibility::Visible);
	ApplyMotion(0.0f);
	return true;
}

bool USkillCutInWidget::StartCutIn(const FSkillCutInPresentationData& Presentation)
{
	return PlayCutIn(Presentation);
}

void USkillCutInWidget::StopCutIn(const bool bNotifyCompletion)
{
	if (bNotifyCompletion && bCutInPlaying)
	{
		FinishCutIn();
		return;
	}

	bCutInPlaying = false;
	ElapsedSeconds = 0.0f;
	FinishedCallback.Unbind();
	SetVisibility(ESlateVisibility::Collapsed);
	ResetLayerTransforms();
}

void USkillCutInWidget::NativeTick(
	const FGeometry& MyGeometry,
	const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bCutInPlaying == false)
	{
		return;
	}

	ElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	const double RealElapsedSeconds = FPlatformTime::Seconds() - StartedAtRealTimeSeconds;
	const float NormalizedTime = FMath::Clamp(
		ElapsedSeconds / FMath::Max(0.01f, ActiveDurationSeconds), 0.0f, 1.0f);
	ApplyMotion(NormalizedTime);

	if (ElapsedSeconds >= ActiveDurationSeconds || RealElapsedSeconds >= ActiveFailSafeSeconds)
	{
		FinishCutIn();
	}
}

void USkillCutInWidget::ApplyMotion(const float NormalizedTime)
{
	const float T = FMath::Clamp(NormalizedTime, 0.0f, 1.0f);
	const float Direction = ActivePresentation.bMirror ? -1.0f : 1.0f;
	const float ViewportScale = FMath::Max(
		KINDA_SMALL_NUMBER, UWidgetLayoutLibrary::GetViewportScale(this));
	// Canvas offsets and render transforms are Slate units, while GetViewportSize
	// returns physical pixels. Normalize by DPI so the 16:9 panel stays correct.
	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScale;
	UpdatePanelLayoutForViewport(ViewportSize);
	const float OffscreenTravel = FMath::Max(1100.0f, ViewportSize.X * 0.65f);
	const bool bSingleImageRig = ActivePresentation.LayerRig == ESkillCutInLayerRig::MasterDuelSingle;
	const float ActiveEnterEnd = bSingleImageRig ? 0.22f : EnterEnd;
	const float ActiveSettleEnd = bSingleImageRig ? 0.34f : SettleEnd;
	const float ActiveExitStart = bSingleImageRig ? 0.78f : ExitStart;

	float PanelX = 0.0f;
	float PanelOpacity = 1.0f;
	if (T < ActiveEnterEnd)
	{
		const float Enter = EaseOutCubic(T / ActiveEnterEnd);
		PanelX = FMath::Lerp(Direction * OffscreenTravel, -Direction * 42.0f, Enter);
	}
	else if (T < ActiveSettleEnd)
	{
		const float Settle = Smooth01((T - ActiveEnterEnd) / (ActiveSettleEnd - ActiveEnterEnd));
		PanelX = FMath::Lerp(-Direction * 42.0f, 0.0f, Settle);
	}
	else if (T >= ActiveExitStart)
	{
		const float Exit = Smooth01((T - ActiveExitStart) / (1.0f - ActiveExitStart));
		// The player V2 holds at center and disappears in place. Monster and the
		// retained layered prototype keep their original directional exit.
		PanelX = bSingleImageRig
			? 0.0f
			: FMath::Lerp(0.0f, -Direction * OffscreenTravel, Exit);
		PanelOpacity = 1.0f - (bSingleImageRig
			? Exit
			: Smooth01(FMath::Clamp((Exit - 0.35f) / 0.65f, 0.0f, 1.0f)));
	}

	PanelCanvas->SetRenderTranslation(FVector2D(PanelX, 0.0f));
	PanelCanvas->SetRenderOpacity(PanelOpacity);
	PanelCanvas->SetRenderTransformAngle(
		bSingleImageRig ? 0.0f : (ActivePresentation.bMirror ? 4.0f : -4.0f));
	// Fixed canvases participate in visibility only. They never inherit PanelX,
	// caster rotation, squash or overshoot.
	FixedBackgroundCanvas->SetRenderTranslation(FVector2D::ZeroVector);
	FixedBackgroundCanvas->SetRenderOpacity(PanelOpacity);
	FixedBackgroundCanvas->SetRenderTransformAngle(0.0f);
	FixedFrontFXCanvas->SetRenderTranslation(FVector2D::ZeroVector);
	FixedFrontFXCanvas->SetRenderOpacity(PanelOpacity);
	FixedFrontFXCanvas->SetRenderTransformAngle(0.0f);

	const float EnterProgress = EaseOutCubic(FMath::Clamp(T / ActiveEnterEnd, 0.0f, 1.0f));
	const float SettleProgress = Smooth01(FMath::Clamp(
		(T - ActiveEnterEnd) / (ActiveSettleEnd - ActiveEnterEnd), 0.0f, 1.0f));
	const float CharacterX = Direction * FMath::Lerp(150.0f, 0.0f, EnterProgress);

	const bool bMercenaryRig = ActivePresentation.LayerRig == ESkillCutInLayerRig::MercenaryBlade;
	const bool bHumanRig = bMercenaryRig || bSingleImageRig;
	FVector2D BodyScale(1.0f, 1.0f);
	if (T < ActiveEnterEnd)
	{
		BodyScale.X = bHumanRig
			? FMath::Lerp(0.92f, 1.05f, EnterProgress)
			: FMath::Lerp(0.76f, 1.18f, EnterProgress);
		BodyScale.Y = bHumanRig
			? FMath::Lerp(1.08f, 0.96f, EnterProgress)
			: FMath::Lerp(1.24f, 0.86f, EnterProgress);
	}
	else if (T < ActiveSettleEnd)
	{
		BodyScale.X = FMath::Lerp(bHumanRig ? 1.05f : 1.18f, 1.0f, SettleProgress);
		BodyScale.Y = FMath::Lerp(bHumanRig ? 0.96f : 0.86f, 1.0f, SettleProgress);
	}

	const FVector2D BodyTranslation(CharacterX, 6.0f * FMath::Sin(T * UE_TWO_PI));
	BodyLayer->SetRenderTranslation(BodyTranslation);
	BodyLayer->SetRenderScale(BodyScale);
	RimLayer->SetRenderTranslation(BodyTranslation);
	RimLayer->SetRenderScale(BodyScale * 1.015f);

	if (bSingleImageRig)
	{
		// The approved direction uses one generated caster plate, one restrained
		// afterimage and one very brief close-up. The fixed background and impact
		// never inherit these transforms; exit is an in-place fade with no rewind.
		const float Reveal = Smooth01(FMath::Clamp((T - 0.12f) / 0.17f, 0.0f, 1.0f));
		const float ImpactArrival = Smooth01(FMath::Clamp((T - 0.23f) / 0.12f, 0.0f, 1.0f));
		const FVector2D HeroTranslation(
			Direction * FMath::Lerp(160.0f, -7.0f, Reveal),
			-7.0f * ImpactArrival);
		const float HeroAngle = Direction * (
			FMath::Lerp(-4.5f, 0.0f, Reveal) + 1.25f * ImpactArrival);
		const FVector2D HeroScale(
			FMath::Lerp(0.90f, 1.0f, Reveal) + 0.045f * ImpactArrival,
			FMath::Lerp(1.05f, 1.0f, Reveal) + 0.045f * ImpactArrival);

		BodyLayer->SetRenderTranslation(HeroTranslation);
		BodyLayer->SetRenderScale(HeroScale);
		BodyLayer->SetRenderTransformAngle(HeroAngle);
		BodyLayer->SetRenderOpacity(Reveal);

		SingleGhostCyanLayer->SetColorAndOpacity(ActivePresentation.bMirror
			? FLinearColor(0.08f, 0.72f, 1.0f, 1.0f)
			: FLinearColor(1.0f, 0.08f, 0.32f, 1.0f));
		SingleGhostCyanLayer->SetRenderTranslation(HeroTranslation + FVector2D(Direction * 62.0f, 9.0f));
		SingleGhostGoldLayer->SetRenderTranslation(HeroTranslation + FVector2D(Direction * 31.0f, 4.0f));
		SingleGhostCyanLayer->SetRenderScale(HeroScale);
		SingleGhostGoldLayer->SetRenderScale(HeroScale);
		SingleGhostCyanLayer->SetRenderTransformAngle(HeroAngle);
		SingleGhostGoldLayer->SetRenderTransformAngle(HeroAngle);
		// The approved frame is the hold composition: keep one readable faction
		// afterimage behind the caster until the entire cut-in fades together.
		SingleGhostCyanLayer->SetRenderOpacity(0.26f * Reveal);
		SingleGhostGoldLayer->SetRenderOpacity(0.0f);

		const float FaceCut = TrianglePulse(T, 0.095f, 0.07f);
		SingleFaceCropLayer->SetRenderTranslation(FVector2D(
			Direction * FMath::Lerp(120.0f, -12.0f, EaseOutCubic(FMath::Clamp(T / 0.15f, 0.0f, 1.0f))), 0.0f));
		SingleFaceCropLayer->SetRenderScale(FVector2D(1.12f, 1.12f));
		SingleFaceCropLayer->SetRenderTransformAngle(-Direction * 3.5f);
		SingleFaceCropLayer->SetRenderOpacity(FaceCut * 0.16f);

		SingleWeaponCropLayer->SetRenderTranslation(FVector2D(
			Direction * FMath::Lerp(90.0f, 10.0f, EaseOutCubic(FMath::Clamp((T - 0.10f) / 0.16f, 0.0f, 1.0f))), 0.0f));
		SingleWeaponCropLayer->SetRenderScale(FVector2D(1.08f, 1.08f));
		SingleWeaponCropLayer->SetRenderTransformAngle(Direction * 3.0f);
		SingleWeaponCropLayer->SetRenderOpacity(0.0f);

		BackgroundLayer->SetRenderTranslation(FVector2D::ZeroVector);
		BackgroundLayer->SetRenderScale(FVector2D(1.0f, 1.0f));
		// This is a world/screen-space effect layer. It stays pinned to the panel;
		// only the caster and its ghost receive the impact translation.
		SpeedLinesLayer->SetRenderTranslation(FVector2D::ZeroVector);
		SpeedLinesLayer->SetRenderOpacity(0.62f * Reveal);
		ForegroundLayer->SetRenderTranslation(FVector2D::ZeroVector);
		ForegroundLayer->SetRenderScale(FVector2D(0.90f + 0.12f * ImpactArrival));
		ForegroundLayer->SetRenderOpacity(ImpactArrival * 0.92f);
	}
	else if (bMercenaryRig)
	{
		// All mercenary art is authored on one 1672x941 canvas.  Attachment tabs
		// remain behind the clean torso, while head, sword arm, shield arm, shield,
		// cape and VFX receive independent Live2D-like secondary motion.
		const float BladeProgress = Smooth01(FMath::Clamp((T - 0.08f) / 0.20f, 0.0f, 1.0f));
		const float HeadProgress = Smooth01(FMath::Clamp((T - 0.055f) / 0.25f, 0.0f, 1.0f));
		const float Recoil = TrianglePulse(T, 0.36f, 0.16f);
		const float CapeWave = FMath::Sin(T * UE_TWO_PI * 2.1f - 1.3f);
		const FVector2D CapeTranslation = BodyTranslation + FVector2D(
			Direction * (-8.0f * CapeWave + 22.0f * (1.0f - EnterProgress)),
			5.0f * CapeWave);
		const FVector2D SwordTranslation = BodyTranslation;
		const FVector2D ShieldArmTranslation = BodyTranslation
			+ FVector2D(-Direction * 7.0f * Recoil, -4.0f * Recoil);
		const FVector2D HeadTranslation = BodyTranslation + FVector2D(
			Direction * 10.0f * (1.0f - HeadProgress), 9.0f * (1.0f - HeadProgress));
		const FVector2D ShieldTranslation = BodyTranslation
			+ FVector2D(-Direction * 12.0f * Recoil, -5.0f * Recoil);
		const float BladeAngle = FMath::Lerp(-Direction * 12.0f, 0.0f, BladeProgress)
			+ Direction * 3.0f * TrianglePulse(T, 0.30f, 0.12f);
		const float HeadAngle = FMath::Lerp(-Direction * 2.8f, 0.0f, HeadProgress)
			+ Direction * 1.6f * TrianglePulse(T, 0.29f, 0.13f);

		MercenaryCapeLayer->SetRenderTransformPivot(FVector2D(0.2172f, 0.3936f));
		MercenarySwordArmLayer->SetRenderTransformPivot(FVector2D(0.2261f, 0.4177f));
		MercenaryShieldArmLayer->SetRenderTransformPivot(FVector2D(0.3551f, 0.4560f));
		BodyLayer->SetRenderTransformPivot(FVector2D(0.2846f, 0.4545f));
		MercenaryHeadLayer->SetRenderTransformPivot(FVector2D(0.2886f, 0.3720f));
		MercenaryShieldLayer->SetRenderTransformPivot(FVector2D(0.3960f, 0.6107f));
		MercenaryCapeLayer->SetRenderTranslation(CapeTranslation);
		MercenaryCapeLayer->SetRenderTransformAngle(
			Direction * (-2.8f * (1.0f - EnterProgress) + 1.4f * CapeWave));
		MercenaryCapeLayer->SetRenderScale(FVector2D(
			1.0f + 0.015f * CapeWave, 1.0f - 0.025f * CapeWave));
		MercenarySwordArmLayer->SetRenderTranslation(SwordTranslation);
		MercenarySwordArmLayer->SetRenderTransformAngle(BladeAngle);
		MercenaryShieldArmLayer->SetRenderTranslation(ShieldArmTranslation);
		MercenaryShieldArmLayer->SetRenderTransformAngle(-Direction * 2.2f * Recoil);
		MercenaryHeadLayer->SetRenderTranslation(HeadTranslation);
		MercenaryHeadLayer->SetRenderTransformAngle(HeadAngle);
		MercenaryShieldLayer->SetRenderTranslation(ShieldTranslation);
		MercenaryShieldLayer->SetRenderTransformAngle(-Direction * 3.8f * Recoil);

		const float ArcPulse = TrianglePulse(T, 0.30f, 0.18f);
		const float ImpactPulse = TrianglePulse(T, 0.335f, 0.085f);
		MercenarySwordArcLayer->SetRenderTranslation(
			BodyTranslation + FVector2D(Direction * FMath::Lerp(50.0f, -35.0f, BladeProgress),
				12.0f * (1.0f - BladeProgress)));
		MercenarySwordArcLayer->SetRenderTransformAngle(Direction * FMath::Lerp(-8.0f, -1.0f, BladeProgress));
		MercenarySwordArcLayer->SetRenderOpacity(0.24f * ArcPulse * PanelOpacity);
		MercenaryImpactLayer->SetRenderTranslation(BodyTranslation + FVector2D(-Direction * 145.0f, -10.0f));
		MercenaryImpactLayer->SetRenderScale(FVector2D(0.38f + 0.10f * ImpactPulse));
		MercenaryImpactLayer->SetRenderOpacity(0.28f * ImpactPulse * PanelOpacity);
	}
	else
	{
		// Monster eye pieces deliberately lag behind the elastic body.
		const float SocketLag = Direction * 38.0f * (1.0f - EnterProgress);
		const float EyeLag = Direction * 62.0f * (1.0f - EnterProgress);
		const FVector2D SocketTranslation = BodyTranslation + FVector2D(SocketLag, 0.0f);
		const FVector2D EyeTranslation = BodyTranslation + FVector2D(EyeLag, 2.0f);
		EyeSocketLayer->SetRenderTranslation(SocketTranslation);
		EyeWhiteLayer->SetRenderTranslation(EyeTranslation);
		IrisLayer->SetRenderTranslation(EyeTranslation + ActivePresentation.PupilAim * 8.0f * Smooth01(T));
		PupilLayer->SetRenderTranslation(EyeTranslation + ActivePresentation.PupilAim * 24.0f * Smooth01(T));

		const FVector2D EyeScale(
			FMath::Lerp(0.92f, 1.0f, EnterProgress),
			FMath::Lerp(1.08f, 1.0f, EnterProgress));
		EyeSocketLayer->SetRenderScale(EyeScale);
		EyeWhiteLayer->SetRenderScale(EyeScale);
		IrisLayer->SetRenderScale(EyeScale * FMath::Lerp(0.94f, 1.0f, Smooth01(T)));
		PupilLayer->SetRenderScale(EyeScale * FMath::Lerp(0.86f, 1.08f, TrianglePulse(T, 0.34f, 0.28f)));
	}

	if (bSingleImageRig == false)
	{
		// Generated opaque plates in the retained layered rigs stay registered to
		// the panel so their parallax cannot expose an edge.
		BackgroundLayer->SetRenderTranslation(FVector2D::ZeroVector);
		SpeedLinesLayer->SetRenderTranslation(FVector2D(-Direction * 330.0f * T, 28.0f * T));
		ForegroundLayer->SetRenderTranslation(FVector2D(-Direction * 560.0f * T, 52.0f * T));
	}
	FrameLayer->SetRenderTranslation(FVector2D(-Direction * 9.0f * (1.0f - EnterProgress), 0.0f));

	const float ImpactPulse = TrianglePulse(T, bSingleImageRig ? 0.34f : 0.27f,
		bSingleImageRig ? 0.10f : 0.12f);
	RimLayer->SetRenderOpacity(FMath::Lerp(0.28f, 1.0f, ImpactPulse) * PanelOpacity);
	FlashLayer->SetRenderOpacity(TrianglePulse(T, bSingleImageRig ? 0.35f : 0.285f,
		bSingleImageRig ? 0.055f : 0.045f) * 0.72f * PanelOpacity);
	AccentWedgeFront->SetRenderOpacity(bSingleImageRig
		? 0.0f
		: ImpactPulse * 0.34f * PanelOpacity);
	AccentWedgeFront->SetRenderTranslation(FVector2D(-Direction * 180.0f * T, 0.0f));
	AccentWedgeBack->SetRenderOpacity(bSingleImageRig ? 0.0f : PanelOpacity);
	AccentWedgeBack->SetRenderTranslation(FVector2D(Direction * 80.0f * (1.0f - EnterProgress), 0.0f));
	DimLayer->SetRenderOpacity(FMath::Min(1.0f, EnterProgress * 1.35f) * PanelOpacity);
}

void USkillCutInWidget::ResetLayerTransforms()
{
	const TArray<UWidget*> Layers = {
		BackgroundLayer, SpeedLinesLayer, RimLayer, MercenaryCapeLayer,
		MercenarySwordArmLayer, MercenaryShieldArmLayer, SingleGhostCyanLayer,
		SingleGhostGoldLayer, BodyLayer,
		MercenaryHeadLayer, MercenaryShieldLayer, EyeSocketLayer,
		EyeWhiteLayer, IrisLayer, PupilLayer, MercenarySwordArcLayer,
		MercenaryImpactLayer, SingleFaceCropLayer, SingleWeaponCropLayer,
		ForegroundLayer, FrameLayer, FlashLayer,
		AccentWedgeBack, SingleGoldWedgeLayer, AccentWedgeFront
	};
	for (UWidget* Layer : Layers)
	{
		if (Layer == nullptr)
		{
			continue;
		}
		Layer->SetRenderTranslation(FVector2D::ZeroVector);
		Layer->SetRenderScale(FVector2D(1.0f, 1.0f));
		Layer->SetRenderOpacity(1.0f);
		Layer->SetRenderTransformAngle(0.0f);
		Layer->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	for (int32 LineIndex = 0; LineIndex < SingleSpeedLineLayers.Num(); ++LineIndex)
	{
		if (UBorder* SpeedLine = SingleSpeedLineLayers[LineIndex])
		{
			SpeedLine->SetRenderTranslation(FVector2D::ZeroVector);
			SpeedLine->SetRenderScale(FVector2D(1.25f, 0.006f + 0.002f * (LineIndex % 3)));
			SpeedLine->SetRenderTransformAngle(-7.0f);
			SpeedLine->SetRenderOpacity(0.0f);
		}
	}

	if (PanelCanvas != nullptr)
	{
		PanelCanvas->SetRenderTranslation(FVector2D::ZeroVector);
		PanelCanvas->SetRenderScale(FVector2D(1.0f, 1.0f));
		PanelCanvas->SetRenderOpacity(1.0f);
	}
	for (UCanvasPanel* FixedCanvas : { FixedBackgroundCanvas.Get(), FixedFrontFXCanvas.Get() })
	{
		if (FixedCanvas != nullptr)
		{
			FixedCanvas->SetRenderTranslation(FVector2D::ZeroVector);
			FixedCanvas->SetRenderScale(FVector2D(1.0f, 1.0f));
			FixedCanvas->SetRenderOpacity(1.0f);
			FixedCanvas->SetRenderTransformAngle(0.0f);
		}
	}
	if (FlashLayer != nullptr)
	{
		FlashLayer->SetRenderOpacity(0.0f);
	}
	if (AccentWedgeFront != nullptr)
	{
		AccentWedgeFront->SetRenderOpacity(0.0f);
	}
	for (UImage* SingleImageLayer : {
		SingleGhostCyanLayer.Get(), SingleGhostGoldLayer.Get(),
		SingleFaceCropLayer.Get(), SingleWeaponCropLayer.Get() })
	{
		if (SingleImageLayer != nullptr)
		{
			SingleImageLayer->SetRenderOpacity(0.0f);
		}
	}
}

void USkillCutInWidget::UpdatePanelLayoutForViewport(const FVector2D& ViewportSize)
{
	if (ViewportSize.X <= 1.0f || ViewportSize.Y <= 1.0f)
	{
		return;
	}

	const bool bPlayerSide = ActivePresentation.LayerRig == ESkillCutInLayerRig::MercenaryBlade
		|| (ActivePresentation.LayerRig == ESkillCutInLayerRig::MasterDuelSingle
			&& ActivePresentation.bMirror);
	const float DesiredWidth = ViewportSize.X * CutInViewportWidthFraction;
	const float DesiredHeight = DesiredWidth / CutInAspectRatio;
	const float MaxHeight = ViewportSize.Y * CutInViewportMaxHeightFraction;
	const float PanelHeight = FMath::Min(DesiredHeight, MaxHeight);
	const float PanelWidth = PanelHeight * CutInAspectRatio;
	const float Left = bPlayerSide ? 0.0f : ViewportSize.X - PanelWidth;
	const float Top = (ViewportSize.Y - PanelHeight) * 0.5f;
	const FMargin PanelOffsets(Left, Top, PanelWidth, PanelHeight);

	for (UCanvasPanel* Canvas : {
		FixedBackgroundCanvas.Get(), PanelCanvas.Get(), FixedFrontFXCanvas.Get() })
	{
		if (UCanvasPanelSlot* CanvasSlot = Canvas != nullptr
			? Cast<UCanvasPanelSlot>(Canvas->Slot) : nullptr)
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetOffsets(PanelOffsets);
		}
	}
}

void USkillCutInWidget::FinishCutIn()
{
	if (bCompletionDispatched)
	{
		return;
	}

	bCompletionDispatched = true;
	bCutInPlaying = false;
	SetVisibility(ESlateVisibility::Collapsed);
	ResetLayerTransforms();

	// Blueprint listeners are still part of presentation cleanup. Notify them
	// before the native HUD callback releases the gameplay barrier and may
	// synchronously re-enter skill execution.
	OnCutInFinished.Broadcast();
	FOnSkillCutInFinished Callback = MoveTemp(FinishedCallback);
	FinishedCallback.Unbind();
	Callback.ExecuteIfBound();
}

void USkillCutInWidget::NativeDestruct()
{
	StopCutIn(false);
	Super::NativeDestruct();
}

void USkillCutInWidget::PlayOpenUIAnimation_Implementation()
{
	FinishOpenUI();
}

void USkillCutInWidget::PlayCloseUIAnimation_Implementation()
{
	FinishCloseUI();
}
