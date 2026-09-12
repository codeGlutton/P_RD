#pragma once
#include "CoreMinimal.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tutorial/FirstPlayProgress.h"
#include "Tutorial/GuidedTutorial.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "FirstPlayTutorialSubsystem.generated.h"
class UUserWidget;
class USRPGAction;
enum class ECombatInputType : uint8;
UCLASS()
class P_RD_API UFirstPlayTutorialSubsystem : public UGameInstanceSubsystem, public IUserDataWriter
{
	GENERATED_BODY()
  public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	bool IsInputRestricted() const;
	bool IsPointerAllowed(const FVector2D& Position) const;
	class UStaticCombatRoomSpawnData* PrepareScenario(class UStaticCombatRoomSpawnData* Original,
		const TArray<TObjectPtr<class UPlayerUnitModel>>& Party);
	FTileIndex GetScenarioMoveTile() const;
	FTileIndex GetScenarioEnemyTile() const;
	bool HasScenario() const { return ScenarioRoom != nullptr; }
	bool IsScenarioGuiding() const;
	int32 GetScenarioSkillIndex() const { return ScenarioSkillIndex; }
	bool IsScenarioTileAllowed(const struct FTileIndex& Tile) const;
	bool IsScenarioPointerAllowed(const FVector2D& Position) const;
	bool IsScenarioCommandAllowed(ECombatInputType Type, int32 Payload) const;
	bool GetScenarioFocus(TArray<FVector2D>& AbsoluteCorners) const;
#if WITH_DEV_AUTOMATION_TESTS
	bool IsWaitingForActionForTest() const { return bExecutingAction; }
#endif
	void PrepareFirstProfile();
	void TitleOpened(UUserWidget* Owner);
	void TitleClosed(UUserWidget* Owner);
	void TurnStarted(bool PlayerTurn);
	void TurnEnded(bool PlayerTurn, bool Succeeded);
	void ActionEnded(bool PlayerAction, const USRPGAction* Action, bool Succeeded);
	void CombatEnded(bool Finished = true);
	void ActionStarted(const USRPGAction* Action);
	void UpdateGuidedHUD(class UCombatLayoutHUDWidget* HUD);
	void AdvanceGuided(EGuidedStage Expected);

  private:
	TSharedPtr<class FTutorialInputGate> InputGate;
	UPROPERTY(Transient) TObjectPtr<class UStaticTutorialRoomSpawnData> ScenarioRoom;
	TWeakObjectPtr<class UPlayerUnitModel> ScenarioUnit;
	int32 ScenarioSkillIndex = INDEX_NONE;
	FText ScenarioSkillName;
	bool bProfileChecked = false;
	bool bInCombat = false;
	bool bPlayerTurn = false;
	bool bExecutingAction = false;
	UPROPERTY(Transient) TObjectPtr<class UGuidedTutorialWidget> GuidedWidget;
	TWeakObjectPtr<class UButton> BoundButton;
	EGuidedStage BoundStage = EGuidedStage::Done;
	UFUNCTION() void GuidedClicked();
	void HideGuided();
	TWeakObjectPtr<class UCombatLayoutHUDWidget> LastHUD;
	EGuidedLesson LastLessonContext = EGuidedLesson::None;

	void Save();
};
