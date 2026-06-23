// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TAS/TacticalAbility.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "TAS/Effect/Stat/TacticalEffectContext_Stat.h"
#include "TacticalAbility_Skill.generated.h"

UCLASS()
class P_RD_API UTacticalEffectPayload_Skill : public UTacticalEffectPayload
{
	GENERATED_BODY()

public:
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

	virtual void EndAbility(const FTacticalAbilityContext& Context);

	virtual bool CanActivateAbility(const FTacticalAbilityContext& Context) const;

private:
	/*
	* @brief  스킬 사용 전 패시브 호출
	* @details
	* 스킬 사용 전은 플레이어의 현재 상태만 안다.
	*/
	void CallStartCalculatePassive(const FTacticalAbilityContext& Context, TArray<UTacticalEffectContext*>& EffectContext);

	/*
	* 스킬 사용 후 패시브 호출
	*/
	void CallEndCalculatePassive(const FTacticalAbilityContext& Context, TArray<UTacticalEffectContext*>& EffectContext);

	/*
	* @brief 효과 계산 함수
	*
	* @todo 추후 계산기로 분리할 수도 있음
	*/
	void ActivateSkill(const FTacticalAbilityContext& Context);






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
