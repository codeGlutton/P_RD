#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "UI/Combat/CombatUITypes.h"

#include "CombatTutorialGuide.generated.h"

class UBorder;
class UButton;
class UCombatTileMapHUDWidget;
class UCombatUIModel;
class UTextBlock;
class UWidget;

/** 첫 전투의 안내 순서. 일반 전투 HUD의 상태와 분리해 튜토리얼 객체만 소유한다. */
enum class ECombatTutorialGuideStep : uint8
{
	None,
	RollDiceBoard,
	WaitDiceResult,
	DismissDiceBoard,
	SelectStepSkill,
	SelectStepDice,
	PreviewStep,
	ConfirmStep,
	WaitStepResolved,
	PressMove,
	PreviewMove,
	ConfirmMove,
	WaitMoveResolved,
	SelectAttackSkill,
	SelectAttackDice,
	PreviewAttack,
	ConfirmAttack,
	WaitAttackResolved,
	SelectBuffSkill,
	SelectBuffDice,
	PreviewBuff,
	ConfirmBuff,
	WaitBuffResolved,
	Complete,
};

/**
 * 첫 전투에서만 생성되는 튜토리얼 안내 객체.
 * 순서 상태, 강조 UI, 허용 입력 전달을 모두 소유해 일반 HUD 로직에 튜토리얼 분기를 남기지 않는다.
 */
UCLASS()
class P_RD_API UCombatTutorialGuide : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UCombatTileMapHUDWidget* InOwner);
	void BindCombatUIModel(UCombatUIModel* InCombatUIModel);
	void BeginDiceRollTutorial();
	void Tick();

protected:
	virtual void BeginDestroy() override;

private:
	UFUNCTION()
	void HandleTargetClicked();

	UFUNCTION()
	void HandleCombatUIChanged(ECombatUIDomain Domain);

	UFUNCTION()
	void HandleActionResolved();

	bool ShouldShow() const;
	bool IsActive() const;
	void SetStep(ECombatTutorialGuideStep Step);
	void RefreshProgress();
	void UpdateLayout();
	void SetVisible(bool bVisible) const;
	void ForwardWorldTouch();

	bool GetWidgetRect(const UWidget* Widget, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetUnitRect(bool bPlayer, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetTileRect(const FTileIndex& Tile, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetTargetRect(FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool ResolveMoveTarget(FTileIndex& OutTarget) const;
	int32 GetDiceIndex() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCombatTileMapHUDWidget> mOwner;

	UPROPERTY(Transient)
	TObjectPtr<UCombatUIModel> mCombatUIModel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> mGuidePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> mGuideText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mInputBlocker;

	UPROPERTY(Transient)
	TObjectPtr<UButton> mTargetButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mScrims;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mOutlineEdges;

	ECombatTutorialGuideStep mStep = ECombatTutorialGuideStep::None;
	FTileIndex mMoveOriginTile = FTileIndex::Invalid;
	FTileIndex mMoveTargetTile = FTileIndex::Invalid;
	int32 mEnemyUnitId = INDEX_NONE;
	float mEnemyHPBeforeAttack = 0.0f;
	float mEnemyDefenseBeforeAttack = 0.0f;
	float mPlayerDefenseBeforeBuff = 0.0f;
	bool mStepEffectObserved = false;
	bool mMoveObserved = false;
	bool mAttackEffectObserved = false;
	bool mBuffEffectObserved = false;
};
