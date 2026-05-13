/*****************************************************************//**
 * @file   RandomStreamFunctionLibrary.h
 * @brief  랜덤 스트림 연관 헬퍼 함수 라이브러리 헤더
 * @author 모호재
 * @date   2026-05-11
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

/**
 * @brief  랜덤 스트림 연관 헬퍼 함수 라이브러리
 */
class P_RD_API URandomStreamFunctionLibrary
{
public:
	static const FRandomStream& GetStageBuildStream(const UObject* WorldContextObject);
	static const FRandomStream& GetEventStream(const UObject* WorldContextObject);
	static const FRandomStream& GetCombatStream(const UObject* WorldContextObject);
};
