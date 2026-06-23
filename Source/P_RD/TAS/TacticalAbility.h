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

public:
	/*
	* @brief Ability를 발동하여 효과를 업데이트 시킨다.
	* 
	* @param Context 기본 효과에 필요한 정보 컨텍스트
	* @param EffectContext 효과값을 갱신한 반환값
	* @param PassiveStackContext 패시브 업데이트 값(실제 적용 X)
	* 
	* @return 패시브 업데이트 여부
	*/
	virtual bool ActivateAbility(
		const FTacticalAbilityContext& Context,
		OUT TArray<class UTacticalEffectContext*>& EffectContext,
		OUT class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(TacticalAbility::ActivateAbility, return false;);

	/*
	* @brief 패시브를 실제로 업데이트 시킨다.
	* 
	* @param PassiveStackContext 패시브 업데이트를 시킬 대상
	* 
	* @return 패시브 업데이트 여부
	*/
	virtual bool UpdatePassive(OUT class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(TacticalAbility::UpdatePassive, return false;);

public:
	/*
	* @brief 스킬을 사용이 가능한지 알려준다.
	*/
	virtual bool CanActivateAbility(
		const FTacticalAbilityContext Context,
		const TArray<class UTacticalEffectContext*>& EffectContext,
		const class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(TacticalAbility::ActivateAbility, return false;);

};
