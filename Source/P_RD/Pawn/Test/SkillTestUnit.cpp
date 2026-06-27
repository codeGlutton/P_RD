// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Test/SkillTestUnit.h"
#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"


ASkillTestUnit::ASkillTestUnit()
{
}

void ASkillTestUnit::ActivateSkill(int32 SkillPoint)
{
	TArray<FTileIndex> Tile;

	// WARING by Mohojae : 
	// 
	// Model을 소환해서 Actor가 자동 배치되는 방식으로 생성 과정이 결정되어 있는데, 
	// 강제적으로 스폰하면 해당 Model의 초기화 로직은 책임질 수 없음.

	NewObject<UUnitModel>()->FindComponentModelByClass<USkillComponentModel>()->ActivateSkill(0, Tile, 100);
}
