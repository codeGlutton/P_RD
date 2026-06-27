// Fill out your copyright notice in the Description page of Project Settings.


#include "Pawn/Test/SkillTestUnitModel.h"

USkillTestUnitModel::USkillTestUnitModel()
{
	//mAttributeCompModel = CreateDefaultSubobject<UAttributeSetComponentModel>(TEXT("AttributeSetComponentModel"));
	mUnitAttributeSet = CreateDefaultSubobject<UUnitAttributeSet>(TEXT("UnitAttributeSet"));
	//mSkillCompModel = CreateDefaultSubobject<USkillComponentModel>(TEXT("SkillComponentModel"));
}
