// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "RDMinimal.h"
#include "UObject/Object.h"
#pragma region Temp
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#pragma endregion

#include "TacticalAbility.generated.h"

/**
 * @brief 패시브 어빌리티에 의한 이펙트 요청 모음 컨테이너
 * By Mohojae
 */
USTRUCT()
struct FTacticalEffectRequestContainer
{
	GENERATED_BODY()

public:
	/**
	 * @brief	타격 가능한 대상에 대한 변경값 매핑 데이터
	 *
	 * @details
	 *
	 * ## 디테일한 설명
	 * UBoardActorModel 중에서 타격 가능한 객체는 IBoardCombatTarget를 상속해야만 한다.
	 * (여기서 키로 TObjectPtr<IBoardCombatTarget>는 문법상 사용할 수 없기에, 일단 UBoardActorModel로 둔다)
	 * 각 IBoardCombatTarget 타겟들의 변화량을 FBoardCombatTargetSnapshotData 인스턴스에 누적한다.
	 *
	 * 여기서 해당 FBoardCombatTargetSnapshotData는 기존 스냅샷과
	 * operator + 연산이 가능하여 쉽게 다음 패시브 계산 시 쉽게 스냅샷을 덮어 쓸 수 있다.
	 * Actor/BoardActor/BoardCombatTarget.h 정의를 참조할 것
	 */
	TMap<TObjectPtr<UBoardActorModel>, FBoardCombatTargetSnapshotData> mTargetRequests;
	/**
	 * @brief	해당 지역의 스폰 요청 매핑 데이터
	 *
	 * @details
	 *
	 * ## 디테일한 설명
	 * 각 타일 칸에 스폰될 클래스를 담아둔다.
	 * 해당 보드 액터는 한 모션이 처리되는 도중에는 맞지않고, 다음 모션 이전에는 스폰처리해야한다.
	 * 다음 모션에서는 타격에 들어가야되기 때문이다.
	 *
	 *
	 * ## 고려해봐야 할 점
	 * 1. 같은 위치에 중복 스폰하는 경우, 누구를 우선 배치해야되는가이다.
	 * 2. 중요한 점은 밀치는 효과를 처리하기 전에, 먼저 스폰이 우선시 되야한다. 그렇지 않다면 겹칠 여지가 있다.
	 * 다만 반대로 하는 경우에도 타일이 찼는지 여부를 후 순위로 체크하고 생성 구역 내에 모든 요청을 작성해두는 방안이 있다.
	 */
	TMap<FTileIndex, TSubclassOf<UBoardActorModel>> mSpawnRequests;

	/**
	 * NOTE :
	 * 밀리는 효과의 경우는 특정 위치가 아닌, 몇 칸 밀리는지만 기록한다.
	 * 이는 이전 기획에서 언급했듯, 시전자와 멀어지는 방향으로 알아서 처리되기 때문이다.
	 */
};

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
		IN const class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(UTacticalAbility::ActivateAbility, return false;);

	/*
	* @brief 패시브를 실제로 업데이트 시킨다.
	* 
	* @param PassiveStackContext 패시브 업데이트를 시킬 대상
	* 
	* @return bool 패시브가 온전히 갱신되었는지 여부, 업데이트 된 PassiveStackContext
	*/
	virtual bool UpdatePassive(OUT class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(UTacticalAbility::UpdatePassive, return false;);

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
		const class UPassiveStackContext* PassiveStackContext) PURE_VIRTUAL(UTacticalAbility::CanActivateAbility, return false;);

};
