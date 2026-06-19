// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TAS/TacticalAbility.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "TAS/Effect/TacticalEffectContext_Stat.h"
#include "TacticalAbility_Skill.generated.h"

struct FTacticalEffectPayload_Skill : public FTacticalEffectPayload
{
	TSoftObjectPtr<UStaticSkillData> mSkillData;
};

/**
 * 
 */
UCLASS()
class P_RD_API UTacticalAbility_Skill : public UTacticalAbility
{
	GENERATED_BODY()
	
public:
	UTacticalAbility_Skill();

public:
	/*
	* @brief 주사위, 주사위 눈금을 소모한다.
	*/
	void ApplyCost() const;

public:
	virtual void ActivateAbility(const FTacticalAbilityContext Context);

	virtual void EndAbility();

	virtual bool CanActivateAbility(const FTacticalAbilityContext& Context) const;

private:
	/*
	* @brief 효과 계산 함수
	* 
	* @todo 추후 계산기로 분리할 수도 있음
	*/
	TArray<UTacticalEffectContext*> CalculateEffect_Stat(const FTacticalAbilityContext& Context);

	// TacticalAbility용 Task 만들 예정
public:
	/*
	* @brief 모션 재생이 완료되었다.
	*/
	void MotionComplete();

	/*
	* @brief 모션 재생 중 어빌리티에게 효과 적용 요청이 들어왔다.
	*/
	void MotionEffectApplyNotify();
};
