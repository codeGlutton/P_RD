#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "UI/Combat/CombatUIModel.h"

#include "CombatTutorialGuide.generated.h"

class UBorder;
class UButton;
class UCombatTileMapHUDWidget;
class UTextBlock;
class UWidget;

/** 첫 전투의 안내 순서. 실제 전투 입력과 결과를 관찰해 다음 단계로 넘어간다. */
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
	EndRoundOne,
	WaitRoundTwo,
	RollDiceBoardRoundTwo,
	WaitDiceResultRoundTwo,
	DismissDiceBoardRoundTwo,
	SelectHealSkill,
	SelectHealDice,
	PreviewHeal,
	ConfirmHeal,
	WaitHealResolved,
	SelectRetreatStepSkill,
	SelectRetreatStepDice,
	PreviewRetreatStep,
	ConfirmRetreatStep,
	WaitRetreatStepResolved,
	PressRetreatMove,
	PreviewRetreatMove,
	ConfirmRetreatMove,
	WaitRetreatMoveResolved,
	SelectRangedAttackSkill,
	SelectRangedAttackDice,
	PreviewRangedAttack,
	ConfirmRangedAttack,
	WaitRangedAttackResolved,
	Complete,
};

/**
 * 첫 전투 전용 튜토리얼 상태와 오버레이.
 * 강조 구멍에는 프록시 버튼을 두지 않아 실제 HUD/월드 입력이 그대로 처리된다.
 */
UCLASS()
class P_RD_API UCombatTutorialGuide : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(UCombatTileMapHUDWidget* InOwner);
	void BindCombatUIModel(UCombatUIModel* InCombatUIModel);
	void Tick(float InDeltaTime);

protected:
	virtual void BeginDestroy() override;

private:
	UFUNCTION()
	void HandleSkipClicked();

	UFUNCTION()
	void HandleCombatUIChanged(ECombatUIDomain Domain);

	UFUNCTION()
	void HandleActionResolved();

	UFUNCTION()
	void HandlePresentationResolved();

	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	bool ShouldShow() const;
	bool IsActive() const;
	void BeginTutorial();
	void FinishTutorial(bool bSkipped);
	void SetStep(ECombatTutorialGuideStep Step);
	void RefreshProgress();
	void UpdateLayout();
	void SetVisible(bool bVisible) const;

	bool HasSkill(const FPrimaryAssetId& SkillId) const;
	int32 GetSkillDiceCost(const FPrimaryAssetId& SkillId) const;
	FPrimaryAssetId GetSelectedSkillId() const;
	bool GetWidgetRect(const UWidget* Widget, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetUnitRect(bool bPlayer, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetTileRect(const FTileIndex& Tile, FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool GetTargetRect(FVector2D& OutTopLeft, FVector2D& OutBottomRight) const;
	bool ResolveMoveTarget(FTileIndex& OutTarget) const;
	bool ResolveRetreatMoveTarget(FTileIndex& OutTarget) const;
	bool ResolveRangedAimTarget(FTileIndex& OutTarget) const;

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
	TObjectPtr<UButton> mSkipButton;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> mInputBlockers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mScrims;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> mOutlineEdges;

	ECombatTutorialGuideStep mStep = ECombatTutorialGuideStep::None;
	FPrimaryAssetId mAttackSkillId;
	FPrimaryAssetId mStepSkillId;
	FPrimaryAssetId mDefenseSkillId;
	FPrimaryAssetId mHealSkillId;
	FPrimaryAssetId mRangedAttackSkillId;
	FTileIndex mMoveOriginTile = FTileIndex::Invalid;
	FTileIndex mMoveTargetTile = FTileIndex::Invalid;
	FTileIndex mRetreatMoveOriginTile = FTileIndex::Invalid;
	FTileIndex mRetreatMoveTargetTile = FTileIndex::Invalid;
	FTileIndex mRangedAimTargetTile = FTileIndex::Invalid;
	int32 mEnemyUnitId = INDEX_NONE;
	float mEnemyHPBeforeAttack = 0.0f;
	float mEnemyDefenseBeforeAttack = 0.0f;
	float mPlayerDefenseBeforeBuff = 0.0f;
	float mMissingTargetElapsed = 0.0f;
	bool mPresentationResolvedForStep = false;
};
