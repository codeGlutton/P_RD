#pragma once

#include "RDMinimal.h"
#include "UI/RDUserWidget.h"

#include "SkillCutInWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UTexture2D;

UENUM(BlueprintType)
enum class ESkillCutInLayerRig : uint8
{
	MonsterElastic,
	MercenaryBlade,
	/** One character texture reused for a close-up, afterimage and the main plate. */
	MasterDuelSingle,
};

/**
 * A data-only description of one layered skill cut-in.
 *
 * Every art layer uses the same transparent canvas.  Missing layers are legal:
 * the native widget keeps a coloured fallback presentation so gameplay can
 * always fail open while art is being authored or cooked.
 */
USTRUCT(BlueprintType)
struct P_RD_API FSkillCutInPresentationData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> SpeedLinesTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> BodyTexture;

	/** Mercenary puppet layers. They are ignored by MonsterElastic. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenaryCapeTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenarySwordArmTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenaryShieldArmTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenaryHeadTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenaryShieldTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenarySwordArcTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art|Mercenary")
	TSoftObjectPtr<UTexture2D> MercenaryImpactTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> EyeSocketTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> EyeWhiteTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> IrisTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> PupilTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> RimTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> ForegroundTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> FrameTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Art")
	TSoftObjectPtr<UTexture2D> FlashTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion")
	FLinearColor AccentColor = FLinearColor(0.78f, 0.05f, 0.035f, 1.0f);

	/** Normalized direction, clamped to [-1, 1], used for the pupil glance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion")
	FVector2D PupilAim = FVector2D(-0.35f, 0.10f);

	/** Selects how the aligned auxiliary layers are interpreted and animated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion")
	ESkillCutInLayerRig LayerRig = ESkillCutInLayerRig::MonsterElastic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion", meta = (ClampMin = "0.10"))
	float DurationSeconds = 0.60f;

	/** Wall-clock emergency finish time; it is forced to be no shorter than DurationSeconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion", meta = (ClampMin = "0.10"))
	float FailSafeSeconds = 0.90f;

	/** Mirrors entry/exit direction and all layer motion for the player-side preset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Skill Cut-In|Motion")
	bool bMirror = false;
};

DECLARE_DELEGATE(FOnSkillCutInFinished)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSkillCutInFinishedBP);

/**
 * Pure-native, reusable layered cut-in. No Widget Blueprint is required.
 *
 * The body, eye pieces and effects move independently to produce a short
 * Live2D-like parallax/squash animation from otherwise static PNG layers.
 */
UCLASS(BlueprintType, Blueprintable)
class P_RD_API USkillCutInWidget : public URDUserWidget
{
	GENERATED_BODY()

public:
	USkillCutInWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Starts/restarts the cut-in and invokes OnFinished exactly once on natural/fail-safe completion. */
	bool PlayCutIn(const FSkillCutInPresentationData& Presentation,
		FOnSkillCutInFinished OnFinished = FOnSkillCutInFinished());

	/** Blueprint-friendly start path. Completion is reported through OnCutInFinished. */
	UFUNCTION(BlueprintCallable, Category = "UI|Skill Cut-In")
	bool StartCutIn(const FSkillCutInPresentationData& Presentation);

	/** Stops immediately. Optionally reports completion to both native and Blueprint listeners. */
	UFUNCTION(BlueprintCallable, Category = "UI|Skill Cut-In")
	void StopCutIn(bool bNotifyCompletion = false);

	UFUNCTION(BlueprintPure, Category = "UI|Skill Cut-In")
	bool IsCutInPlaying() const { return bCutInPlaying; }

	UPROPERTY(BlueprintAssignable, Category = "UI|Skill Cut-In")
	FOnSkillCutInFinishedBP OnCutInFinished;

protected:
	virtual bool Initialize() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual void PlayOpenUIAnimation_Implementation() override;
	virtual void PlayCloseUIAnimation_Implementation() override;

private:
	void EnsureNativeWidgetTree();
	void ApplyPresentation(const FSkillCutInPresentationData& Presentation);
	void ApplyMotion(float NormalizedTime);
	void FinishCutIn();
	void ResetLayerTransforms();
	void UpdatePanelLayoutForViewport(const FVector2D& ViewportSize);

	UImage* MakeImageLayer(UCanvasPanel* Parent, FName Name, int32 ZOrder);
	void SetLayerTexture(UImage* Layer, const TSoftObjectPtr<UTexture2D>& Texture,
		const FLinearColor& MissingTextureColor);
	void SetLayerUVRegion(UImage* Layer, const FVector2f& Minimum, const FVector2f& Maximum);

private:
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> RootCanvas;
	UPROPERTY(Transient) TObjectPtr<UBorder> DimLayer;
	/** Screen-registered panel; generated background/speed art never inherits caster travel. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> FixedBackgroundCanvas;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PanelCanvas;
	/** Screen-registered foreground impact canvas, kept separate from the caster. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> FixedFrontFXCanvas;
	UPROPERTY(Transient) TObjectPtr<UBorder> PanelFallback;
	UPROPERTY(Transient) TObjectPtr<UBorder> AccentWedgeBack;
	/** Code-native streaks used only by the one-texture preset. */
	UPROPERTY(Transient) TArray<TObjectPtr<UBorder>> SingleSpeedLineLayers;
	UPROPERTY(Transient) TObjectPtr<UBorder> SingleGoldWedgeLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> BackgroundLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> SpeedLinesLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> RimLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenaryCapeLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenarySwordArmLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenaryShieldArmLayer;
	/** Faction-tinted motion echoes; both deliberately point at BodyTexture. */
	UPROPERTY(Transient) TObjectPtr<UImage> SingleGhostCyanLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> SingleGhostGoldLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> BodyLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenaryHeadLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenaryShieldLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> EyeSocketLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> EyeWhiteLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> IrisLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> PupilLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenarySwordArcLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> MercenaryImpactLayer;
	/** Fast virtual inserts sampled from BodyTexture via Slate UV regions. */
	UPROPERTY(Transient) TObjectPtr<UImage> SingleFaceCropLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> SingleWeaponCropLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> ForegroundLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> FrameLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> FlashLayer;
	UPROPERTY(Transient) TObjectPtr<UBorder> AccentWedgeFront;

	FOnSkillCutInFinished FinishedCallback;
	FSkillCutInPresentationData ActivePresentation;
	float ElapsedSeconds = 0.0f;
	double StartedAtRealTimeSeconds = 0.0;
	float ActiveDurationSeconds = 0.60f;
	float ActiveFailSafeSeconds = 0.90f;
	bool bCutInPlaying = false;
	bool bCompletionDispatched = false;
};
