/*****************************************************************//**
 * @file   ObjectView.h
 * @brief  시뮬레이션 뷰 최상위 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "ObjectView.generated.h"

UINTERFACE(MinimalAPI)
class UObjectView : public UInterface
{
	GENERATED_BODY()
};

/**
 * @brief  시뮬레이션 뷰의 최상위 인터페이스
 * @details 모든 하위 뷰(보드 액터, 컴포넌트 등)가 구현하는 공통 인터페이스다.
 */
class P_RD_API IObjectView
{
	GENERATED_BODY()

public:
};
