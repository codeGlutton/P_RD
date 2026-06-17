/*****************************************************************//**
 * @file   ComponentModel.h
 * @brief  보드 액터에 부착되는 컴포넌트 데이터 모델 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "Model/ObjectModel.h"
#include "ComponentModel.generated.h"

UINTERFACE(MinimalAPI)
class UComponentModel : public UObjectModel
{
	GENERATED_BODY()
};

/**
 * @brief  보드 액터에 부착되는 컴포넌트 데이터 모델 인터페이스
 * @details 패시브, 장비 등 보드 액터가 소유하는 컴포넌트 모델의 공통 인터페이스다.
 */
class P_RD_API IComponentModel : public IObjectModel
{
	GENERATED_BODY()

public:
};
