// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Test/SkillTestUnit.h"
#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"


ASkillTestUnit::ASkillTestUnit()
{
	//mUnitModel = NewObject<UUnitModel>();
}

void ASkillTestUnit::ActivateSkill(int32 SkillPoint)
{
	TArray<FTileIndex> Tile;
	NewObject<UUnitModel>()->FindComponentModelByClass<USkillComponentModel>()->ActivateSkill(0, Tile, 100);
}
