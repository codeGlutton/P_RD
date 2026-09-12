#pragma once
#include "CoreMinimal.h"
#include "GuidedTutorial.generated.h"

UENUM()
enum class EGuidedStage : uint8
{
	OpenSkills,
	HoldSkill,
	CloseSkill,
	SelectMove,
	MoveTile,
	ConfirmMove,
	SelectSkill,
	SkillTarget,
	ConfirmSkill,
	EndTurn,
	OpenMercenary,
	SelectMercenary,
	MercenarySkill,
	CloseMercenarySkill,
	OpenInventory,
	Artifact,
	CloseArtifact,
	CloseMercenary,
	OpenMonster,
	SelectMonster,
	HoldMonsterSkill,
	CloseMonsterSkill,
	CloseMonster,
	Done,
	LessonMenu,
	ReadTurnOrder,
	ReadAP,
	ReadCondition,
	ReadMercenaryStats,
	ReadSkillCost,
	ReadSkillCooldown,
	SelectRange,
	EffectRange,
	ReadInventory,
	ReadEnemy,
	ReadEnemySkill,
	ReadStatus
};
UENUM()
enum class EGuidedLesson : uint8
{
	None,
	Mercenary,
	Monster,
	Artifact
};

USTRUCT()
struct P_RD_API FGuidedTutorialProgress
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) EGuidedStage Stage = EGuidedStage::OpenSkills;
	// Defaults stay false for existing saves which predate this onboarding flow.
	UPROPERTY(SaveGame) bool Enrolled = false;
	UPROPERTY(SaveGame) bool FirstBattleFinished = false;
	UPROPERTY(SaveGame) bool CoreComplete = false;
	UPROPERTY(SaveGame) bool SystemsComplete = false;
	UPROPERTY(SaveGame) bool SystemsActive = false;
	static bool IsReadingStep(EGuidedStage S)
	{
		switch (S)
		{
		case EGuidedStage::ReadTurnOrder:
		case EGuidedStage::ReadAP:
		case EGuidedStage::ReadCondition:
		case EGuidedStage::ReadMercenaryStats:
		case EGuidedStage::ReadSkillCost:
		case EGuidedStage::ReadSkillCooldown:
		case EGuidedStage::ReadInventory:
		case EGuidedStage::ReadEnemy:
		case EGuidedStage::ReadEnemySkill:
		case EGuidedStage::ReadStatus:
			return true;
		default:
			return false;
		}
	}
	bool BeginSystems()
	{
		if (!Enrolled || SystemsComplete || SystemsActive)
			return false;
		SystemsActive = true;
		Lesson = EGuidedLesson::None;
		Stage = EGuidedStage::ReadTurnOrder;
		return true;
	}
	void FinishSystems()
	{
		SystemsComplete = true;
		SystemsActive = false;
		// The information panels are closed; continue directly with the fixed combat exercise.
		Stage = CanGuideCombat() ? EGuidedStage::OpenSkills : EGuidedStage::LessonMenu;
		Lesson = EGuidedLesson::None;
	}
	bool EnrollIfNoPlayData(bool HasPlaySave)
	{
		if (HasPlaySave || Enrolled)
			return false;
		Enrolled = true;
		return true;
	}
	bool NeedsFirstRoom(bool Skipped) const
	{
		return Enrolled && !Skipped && !CoreComplete && !FirstBattleFinished;
	}
	bool CanGuideCombat() const
	{
		return Enrolled && !FirstBattleFinished && !CoreComplete && !SystemsActive;
	}
	void FinishFirstBattle()
	{
		if (!Enrolled)
			return;
		FirstBattleFinished = true;
		SystemsActive = false;
		Stage = EGuidedStage::LessonMenu;
		Lesson = EGuidedLesson::None;
	}
	UPROPERTY(SaveGame) uint8 LessonsRead = 0;
	UPROPERTY(SaveGame) EGuidedLesson Lesson = EGuidedLesson::None;
	static uint8 LessonBit(EGuidedLesson L)
	{
		return L == EGuidedLesson::None ? 0 : 1 << (uint8(L) - 1);
	}
	bool HasLearned(EGuidedLesson L) const
	{
		return (LessonsRead & LessonBit(L)) != 0;
	}
	static EGuidedStage NextStage(EGuidedStage S)
	{
		switch (S)
		{
		case EGuidedStage::OpenSkills:
			return EGuidedStage::SelectMove;
		case EGuidedStage::CloseSkill:
			return EGuidedStage::SelectSkill;
		case EGuidedStage::EndTurn:
		case EGuidedStage::CloseMercenary:
		case EGuidedStage::CloseMonster:
			return EGuidedStage::LessonMenu;
		case EGuidedStage::CloseMercenarySkill:
			return EGuidedStage::CloseMercenary;
		case EGuidedStage::Done:
		case EGuidedStage::LessonMenu:
			return S;
		default:
			return static_cast<EGuidedStage>(uint8(S) + 1);
		}
	}
	bool Advance(EGuidedStage Expected)
	{
		if (Stage != Expected || Stage == EGuidedStage::Done || Stage == EGuidedStage::LessonMenu)
			return false;
		if (SystemsActive)
		{
			switch (Stage)
			{
			case EGuidedStage::ReadTurnOrder:
				Stage = EGuidedStage::ReadAP;
				break;
			case EGuidedStage::ReadAP:
				Stage = EGuidedStage::ReadCondition;
				break;
			case EGuidedStage::ReadCondition:
				Stage = EGuidedStage::ReadStatus;
				break;
			case EGuidedStage::ReadStatus:
				Stage = EGuidedStage::OpenMercenary;
				break;
			case EGuidedStage::SelectMercenary:
				Stage = EGuidedStage::ReadMercenaryStats;
				break;
			case EGuidedStage::ReadMercenaryStats:
				Stage = EGuidedStage::MercenarySkill;
				break;
			case EGuidedStage::MercenarySkill:
				Stage = EGuidedStage::ReadSkillCost;
				break;
			case EGuidedStage::ReadSkillCost:
				Stage = EGuidedStage::ReadSkillCooldown;
				break;
			case EGuidedStage::ReadSkillCooldown:
				Stage = EGuidedStage::SelectRange;
				break;
			case EGuidedStage::SelectRange:
				Stage = EGuidedStage::EffectRange;
				break;
			case EGuidedStage::EffectRange:
				Stage = EGuidedStage::CloseMercenarySkill;
				break;
			case EGuidedStage::CloseMercenarySkill:
				Stage = EGuidedStage::OpenInventory;
				break;
			case EGuidedStage::OpenInventory:
				Stage = EGuidedStage::ReadInventory;
				break;
			case EGuidedStage::ReadInventory:
				Stage = EGuidedStage::Artifact;
				break;
			case EGuidedStage::CloseArtifact:
				LessonsRead |= LessonBit(EGuidedLesson::Artifact);
				Stage = EGuidedStage::CloseMercenary;
				break;
			case EGuidedStage::CloseMercenary:
				LessonsRead |= LessonBit(EGuidedLesson::Mercenary);
				Stage = EGuidedStage::OpenMonster;
				break;
			case EGuidedStage::SelectMonster:
				Stage = EGuidedStage::ReadEnemy;
				break;
			case EGuidedStage::ReadEnemy:
				Stage = EGuidedStage::HoldMonsterSkill;
				break;
			case EGuidedStage::HoldMonsterSkill:
				Stage = EGuidedStage::ReadEnemySkill;
				break;
			case EGuidedStage::ReadEnemySkill:
				Stage = EGuidedStage::CloseMonsterSkill;
				break;
			case EGuidedStage::CloseMonster:
				LessonsRead |= LessonBit(EGuidedLesson::Monster);
				FinishSystems();
				break;
			default:
				Stage = NextStage(Stage);
				break;
			}
			return true;
		}
		if (Stage == EGuidedStage::EndTurn)
			CoreComplete = true;
		if (Stage == EGuidedStage::CloseMercenary || Stage == EGuidedStage::CloseMonster)
		{
			LessonsRead |= LessonBit(Lesson);
			Lesson = EGuidedLesson::None;
		}
		Stage = Stage == EGuidedStage::OpenMercenary && Lesson == EGuidedLesson::Artifact ? EGuidedStage::OpenInventory
																						  : NextStage(Stage);
		return true;
	}
	bool BeginLesson(EGuidedLesson L, bool HasArtifact)
	{
		if (SystemsActive || !Enrolled || (!CoreComplete && !FirstBattleFinished) || L == EGuidedLesson::None ||
			HasLearned(L) || (L == EGuidedLesson::Artifact && !HasArtifact))
			return false;
		Lesson = L;
		Stage = L == EGuidedLesson::Monster ? EGuidedStage::OpenMonster : EGuidedStage::OpenMercenary;
		return true;
	}
	bool CompleteAction(bool Move, bool Player, bool Succeeded)
	{
		if (!Player || !Succeeded)
			return false;
		if (Move && Stage >= EGuidedStage::SelectMove && Stage <= EGuidedStage::ConfirmMove)
		{
			Stage = EGuidedStage::HoldSkill;
			return true;
		}
		if (!Move && Stage >= EGuidedStage::SelectSkill && Stage <= EGuidedStage::ConfirmSkill)
		{
			Stage = EGuidedStage::EndTurn;
			return true;
		}
		return false;
	}
	void ResumeInNewBattle()
	{
		SystemsActive = false;
		// Old completed saves remain complete; old information checkpoints become optional lessons.
		if (Stage == EGuidedStage::Done)
		{
			CoreComplete = true;
			LessonsRead = 7;
			Stage = EGuidedStage::LessonMenu;
			return;
		}
		// Appended information stages must not be mistaken for completed combat checkpoints.
		if (Enrolled && !SystemsComplete && !FirstBattleFinished)
		{
			Stage = EGuidedStage::ReadTurnOrder;
			Lesson = EGuidedLesson::None;
			return;
		}
		if (CoreComplete || FirstBattleFinished)
		{
			Stage = EGuidedStage::LessonMenu;
			Lesson = EGuidedLesson::None;
			return;
		}
		switch (Stage)
		{
		case EGuidedStage::MoveTile:
		case EGuidedStage::ConfirmMove:
			Stage = EGuidedStage::SelectMove;
			break;
		case EGuidedStage::CloseSkill:
			Stage = EGuidedStage::HoldSkill;
			break;
		case EGuidedStage::SkillTarget:
		case EGuidedStage::ConfirmSkill:
			Stage = EGuidedStage::SelectSkill;
			break;
		default:
			if (Stage >= EGuidedStage::OpenMercenary)
			{
				CoreComplete = true;
				Stage = EGuidedStage::LessonMenu;
			}
			break;
		}
	}
};
