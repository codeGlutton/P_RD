// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "UObject/Object.h"
#pragma region Temp
#include "Actor/BoardActor/BoardActorModel.h"
#pragma endregion

#include "TacticalAbility.generated.h"

/*
* @param SkillIndex : 변경된 스킬의 인덱스
* @param SkillData : 변경된 스킬 데이터
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnTacticalAbility,
	TWeakObjectPtr<UBoardActorModel>, CasterActor
);

enum class ETacticalEffectPayloadType
{
	None,		
	Skill,		// 스킬
	Passive,	// 패시브
	Area		// 장판
};


UCLASS(Abstract)
class P_RD_API UTacticalEffectPayload : public UObject
{
	GENERATED_BODY()

public:
	ETacticalEffectPayloadType mTacticalEffectPayloadType;
};


struct FTacticalAbilityContext
{
	TWeakObjectPtr<UBoardActorModel> mCasterActor;
	TArray<FTileIndex> mTargetTile;

	TObjectPtr<UTacticalEffectPayload> mInstigatorData;	// 수정 필요 ==================================================================
};

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class P_RD_API UTacticalAbility : public UObject
{
	GENERATED_BODY()

public:
	// 시작 델리게이트
	FOnTacticalAbility OnStartAbility;

	// 효과 적용 델리게이트(또는 효과 적용 해)
	FOnTacticalAbility OnApplyAbility;

	// 종료 델리게이트
	FOnTacticalAbility OnEndAbility;

public:
	/*
	* @brief 스킬을 시전한다.
	*/
	virtual void ActivateAbility(const FTacticalAbilityContext Context);

	void ApplyEffect(
		const FTacticalAbilityContext& Context,
		TArray<class UTacticalEffectContext*>& EffectContext);

	/*
	* @brief 스킬을 종료한다.
	*/
	virtual void EndAbility(const FTacticalAbilityContext& Context);

public:
	/*
	* @brief 스킬을 사용이 가능한지 알려준다.
	*/
	virtual bool CanActivateAbility(const FTacticalAbilityContext& Context) const { return true; }

};
