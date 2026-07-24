/*****************************************************************//**
 * @file   RDMinimal.h
 * @brief  RD 프로젝트 전 영역에서 사용되는 최소한의 포함 헤더
 * @details
 * 모든 영역에서 사용되는 헤더를 미리 정의하여, 중복 작성을 방지합니다. \n
 * 단, 해당 영역에 불필요한 헤더를 삽입 시에 컴파일 시간이 길어질 수 있기 때문에 반드시 필요한 항목만 추가합니다.
 * @author 모호재
 * @date   2026-04-23
 *********************************************************************/

#pragma once

/* RD 프로젝트 연관 기본 선언 포함 헤더 */

#include "P_RD.h"

/* 기본 엔진 헤더 */

#include "Engine.h"
#include "Engine/EngineTypes.h"
#include "EngineMinimal.h"

/* 기본 수학 라이브러리 */

#include "Kismet/KismetMathLibrary.h"
#include "Algo/RandomShuffle.h"

/* 게임플레이 태그 헤더 */

#include "NativeGameplayTags.h"
#include "GameplayTagContainer.h" 
#include "GameplayTagType.h"

class UObjectModel;
class UObjectModelFactory;
class UEventLogger;

/* 열거형 연관 */

template<typename T>
FString EnumToString(T Value)
{
    static_assert(TIsEnum<T>::Value);
    return StaticEnum<T>()->GetNameStringByValue((int64)Value);
}

template<typename T>
FString EnumToFullString(T Value)
{
    static_assert(TIsEnum<T>::Value);
    return UEnum::GetValueAsString(Value);
}

template<typename T>
FText EnumToText(T Value)
{
    static_assert(TIsEnum<T>::Value);
    return UEnum::GetDisplayValueAsText(Value);
}

template<typename T>
FText EnumToFullText(T Value)
{
    static_assert(TIsEnum<T>::Value);
    return StaticEnum<T>()->GetDisplayNameTextByValue((int64)Value);
}

/* 모델 연관 */

UObjectModel* GetWorldSubsystemModel(const UObject* WorldContextObject, UClass* Class);

template<typename T>
T* GetWorldSubsystemModel(const UObject* WorldContextObject)
{
    static_assert(TIsDerivedFrom<T, UObjectModel>::IsDerived, "UObjectModel를 상속해야 함");
	return Cast<T>(GetWorldSubsystemModel(WorldContextObject, T::StaticClass()));
}

UObjectModelFactory* GetWorldModelFactory(const UObject* WorldContextObject);

/* 로그 연관 */

UEventLogger* GetWorldEventLogger(const UObject* WorldContextObject);


