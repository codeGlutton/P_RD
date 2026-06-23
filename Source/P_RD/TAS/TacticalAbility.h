// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "UObject/Object.h"
#pragma region Temp
#include "Actor/BoardActor/BoardActorModel.h"
#pragma endregion

#include "TacticalAbility.generated.h"

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
	// 발동하는 정보 타입
	ETacticalEffectPayloadType mTacticalEffectPayloadType;
};


struct FTacticalAbilityContext
{
	// @brief 발동하는 주체
	// 발동하는 주체 (스킬이면 스킬 사용자, 패시브면 패시브 발동자)
	TWeakObjectPtr<UBoardActorModel>	mCasterActor;
	
	// 스킬 또는 패시브 타겟 타일
	TArray<FTileIndex>					mTargetTile;

	// @brief 발동하는 효과의 정보
	// 
	// @details 
	// 스킬 : 스킬 데이터 외 기타
	// 패시브 : 패시브 데이터 외 기타
	TObjectPtr<UTacticalEffectPayload>	mInstigatorData;
};

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class P_RD_API UTacticalAbility : public UObject
{
	GENERATED_BODY()

public:
	/*
	* @brief Ability를 발동하여 효과를 업데이트 시킨다.
	* 
	* @param Context 기본 효과에 필요한 정보 컨텍스트
	* @param EffectContext 효과값을 갱신할 반환값
	* @param PassiveStackContext 패시브 업데이트 값(실제 적용 X)
	* 
	* @return bool 어빌리티 온전히 발동되었는지 여부, 업데이트 된 EffectContext
	*/
	virtual bool ActivateAbility(
		const FTacticalAbilityContext& Context,
		IN OUT TArray<class UTacticalEffectContext*>& EffectContext,
		IN const class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(TacticalAbility::ActivateAbility, return false;);

	/*
	* @brief 패시브를 실제로 업데이트 시킨다.
	* 
	* @param PassiveStackContext 패시브 업데이트를 시킬 대상
	* 
	* @return bool 패시브가 온전히 갱신되었는지 여부, 업데이트 된 PassiveStackContext
	*/
	virtual bool UpdatePassive(OUT class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(TacticalAbility::UpdatePassive, return false;);

public:
	/*
	* @brief 스킬을 사용이 가능한지 알려준다.
	* 
	* @return bool 효과가 발동이 가능한지 여부
	*
	*/
	virtual bool CanActivateAbility(
		const FTacticalAbilityContext Context,
		const TArray<class UTacticalEffectContext*>& EffectContext,
		const class UPassiveStackContext* PassiveStackContext);

};
