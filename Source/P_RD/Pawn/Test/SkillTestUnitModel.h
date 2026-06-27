// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Pawn/UnitModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "AttributeSet/UnitAttributeSet.h"
#include "TAS/Effect/TacticalEffect.h"
#include "SkillTestUnitModel.generated.h"

/**
 * 
 */
UCLASS()
class P_RD_API USkillTestUnitModel : public UUnitModel
{
	GENERATED_BODY()

public:
	USkillTestUnitModel();

public:
	virtual void PreInitializeComponentModels() override {}
	virtual void PostInitializeComponentModels() override {}

public:
	UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "UnitAttributeSet"))
	TObjectPtr<UUnitAttributeSet> mUnitAttributeSet;

	//UPROPERTY(Category = Attribute, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "AttributeCompModel"))
	//TObjectPtr<UAttributeSetComponentModel> mAttributeCompModel;

	//UPROPERTY(Category = Skill, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true", DisplayName = "SkillComp"))
	//TObjectPtr<USkillComponentModel>	mSkillCompModel;
};
