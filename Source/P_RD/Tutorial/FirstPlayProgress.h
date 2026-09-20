#pragma once
#include "CoreMinimal.h"
#include "FirstPlayProgress.generated.h"

UENUM()
enum class EFirstPlayStep : uint8
{
	Welcome,
	Overview,
	Move,
	Skill,
	EndTurn,
	Finished,
	MercenaryInfo,
	MonsterInfo,
	InventoryInfo
};
UENUM()
enum class EFirstPlayAction : uint8
{
	Move = 1,
	Skill = 2,
	EndTurn = 4
};

USTRUCT()
struct P_RD_API FFirstPlayProgress
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) bool WelcomeSeen = false;
	UPROPERTY(SaveGame) bool OverviewSeen = false;
	UPROPERTY(SaveGame) uint8 Actions = 0;
	UPROPERTY(SaveGame) bool Completed = false;
	UPROPERTY(SaveGame) bool Skipped = false;
 // Independent of combat completion so existing players can discover new help.
 UPROPERTY(SaveGame) uint8 InfoAcknowledged = 0;
 static uint8 InfoBit(EFirstPlayStep Step)
 {
  switch (Step) {
  case EFirstPlayStep::MercenaryInfo: return 1;
  case EFirstPlayStep::MonsterInfo: return 2;
  case EFirstPlayStep::InventoryInfo: return 4;
  default: return 0;
  }
 }
 bool NeedsInfo(EFirstPlayStep Step) const { return !Skipped && InfoBit(Step) && !(InfoAcknowledged & InfoBit(Step)); }
 void AcknowledgeInfo(EFirstPlayStep Step) { InfoAcknowledged |= InfoBit(Step); }
	EFirstPlayStep GetStep() const
	{
		if (Completed)
			return EFirstPlayStep::Finished;
		if (!WelcomeSeen)
			return EFirstPlayStep::Welcome;
		if (!OverviewSeen)
			return EFirstPlayStep::Overview;
		if (!(Actions & uint8(EFirstPlayAction::Move)))
			return EFirstPlayStep::Move;
		if (!(Actions & uint8(EFirstPlayAction::Skill)))
			return EFirstPlayStep::Skill;
		if (!(Actions & uint8(EFirstPlayAction::EndTurn)))
			return EFirstPlayStep::EndTurn;
		return EFirstPlayStep::Finished;
	}
	bool Record(EFirstPlayAction Action, bool PlayerAction = true, bool Succeeded = true)
	{
		if (Completed || !PlayerAction || !Succeeded || (Actions & uint8(Action)))
			return false;
		Actions |= uint8(Action);
		UpdateCompletion();
		return true;
	}
	void UpdateCompletion()
	{
		Completed = WelcomeSeen && OverviewSeen && (Actions & 7) == 7;
	}
	void Skip()
	{
		Completed = true;
		Skipped = true;
	}
};
