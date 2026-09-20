#pragma once
#include "Pawn/Player/PlayerUnitModel.h"
#include "LevelUpSkillTestPlayer.generated.h"

UCLASS()
class ULevelUpSkillTestPlayer : public UPlayerUnitModel
{
	GENERATED_BODY()
public:
	EUnitJobType Job = EUnitJobType::Knight;
	EUnitJobType GetUnitJobType() const override { return Job; }
};
