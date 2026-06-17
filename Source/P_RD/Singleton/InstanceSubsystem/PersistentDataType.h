/*****************************************************************//**
 * @file   PersistentDataType.h
 * @brief  영구적 플레이 데이터 연관 타입들 구현 헤더
 * @author 모호재
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "PersistentDataType.generated.h"

/**
 * 게임 볼륨 종류 열거형
 */
UENUM(BlueprintType)
enum class EGameVolumeType : uint8
{
	Master = 0,
	BGM,
	SFX,
	Voice,
	Count,
};

/**
 * 언어 종류 열거형
 */
UENUM(BlueprintType)
enum class ELanguageType : uint8
{
	KOREAN = 0,
	ENGLISH,
	Count,
};

