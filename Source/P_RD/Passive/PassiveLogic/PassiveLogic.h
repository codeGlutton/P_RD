// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PassiveLogic.generated.h"

class UStaticPassiveData;
class UDynamicPassiveData_Base;
class TacticalEffectContextHandle;	// 소유자, 유발자?, 효과 등

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class P_RD_API UPassiveLogic : public UObject
{
	GENERATED_BODY()

protected:
	/**
	 * @brief 패시브 장착 시 
	 * 
	 * @details
	 * 패시브 장착 시 호출
	 * 
	 * @param StaticPassiveData : 패시브 정적 데이터
	 * @param TacticalEffectContextHandle : 장착한 소유자
	 */
	virtual void OnEquip(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData,
		TacticalEffectContextHandle& TacticalEffectContextHandle) {};

	/**
	 * @brief 패시브 해제 시
	 * 
	 * @details
	 * 패시브 해제 시 호출
	 * 
	 * @param StaticPassiveData : 패시브 정적 데이터
	 * @param TacticalEffectContextHandle : 해제한 소유자
	 */
	virtual void OnUnEquip(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData,
		TacticalEffectContextHandle& TacticalEffectContextHandle) {};

	/**
	 * @brief 패시브 호출
	 * 
	 * @details
	 * 패시브 호출
	 * 
	 * @param StaticPassiveData : 패시브 정적 데이터
	 * @param DynamicPassiveData : 패시브 동적 데이터
	 * @param TacticalEffectContextHandle : 해제한 소유자
	 */
	virtual void OnExcute(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData, 
		TObjectPtr<UDynamicPassiveData_Base> DynamicPassiveData,
		TacticalEffectContextHandle& TacticalEffectContextHandle) {};
};
