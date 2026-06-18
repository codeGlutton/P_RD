// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PassiveLogic.generated.h"

class UStaticPassiveData;
class UDynamicPassiveData_Base;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class P_RD_API UPassiveLogic : public UObject
{
	GENERATED_BODY()

protected:
	/**
	 * @brief 오버랩 시작 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnEquip(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData, TObjectPtr<UDynamicPassiveData_Base> DynamicPassiveData) {};

	/**
	 * @brief 오버랩 시작 시 실행될 함수
	 * @param CurTile 현재 위치한 타일 객체
	 * @param Other 반대 대상
	 */
	virtual void OnUnEquip(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData, TObjectPtr<UDynamicPassiveData_Base> DynamicPassiveData) {};

	/**
	* @brief 오버랩 시작 시 실행될 함수
	* @param Tactical Effect 
	* @param Other 반대 대상
	*/
	virtual void OnExcute(TSoftObjectPtr<UStaticPassiveData> StaticPassiveData, TObjectPtr<UDynamicPassiveData_Base> DynamicPassiveData) {};
};
