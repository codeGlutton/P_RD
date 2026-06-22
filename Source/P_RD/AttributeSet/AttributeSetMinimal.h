/*****************************************************************//**
 * @file   AttributeSetMinimal.h
 * @brief  AttributeSet 활용 영역에서 사용되는 포함 헤더
 * @author 모호재
 * @date   2026-04-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

#include "GameplayAbilitiesModule.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "AttributeSet.h"

/**
 * @brief Gameplay Attribute의 Get/Set 등의 접근 함수 자동 정의 매크로
 * @param ClassName AttributeSet 파생 클래스명
 * @param PropertyName Attribute명
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)				\
		GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName)	\
		GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName)				\
		GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName)				\
		GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

