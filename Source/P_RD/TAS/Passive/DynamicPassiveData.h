/*****************************************************************//**
 * @file   DynamicPassiveData.h
 * @brief  패시브 런타임 상태를 저장하는 베이스 struct
 * @author 이문환
 * @date   2026-06-24
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DynamicPassiveData.generated.h"

/**
 * @brief 패시브 런타임 상태 베이스
 *
 * @details
 * 패시브마다 보관하는 런타임 상태(예: 스택 카운터, 쿨다운 턴 수)는 각자 다르므로
 * 이 베이스를 상속한 구체 struct로 각자 정의, TInstancedStruct<FDynamicPassiveData>로 전달,
 * 사용 시 구체 struct로 캐스팅.
 */
USTRUCT()
struct FDynamicPassiveData
{
	GENERATED_BODY()
};
