/*****************************************************************//**
 * @file   BoardActorModel.h
 * @brief  보드에 배치되는 액터 데이터 모델 인터페이스 정의 헤더
 * @author 이문환
 * @date   2026-06-17
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UObject/Interface.h"
#include "Model/ObjectModel.h"
#include "BoardActorModel.generated.h"

UINTERFACE(MinimalAPI)
class UBoardActorModel : public UObjectModel
{
	GENERATED_BODY()
};

/**
 * @brief  보드에 배치되는 액터 데이터 모델 인터페이스
 * @details 패시브, 장비 등 컴포넌트 모델을 소유하는 보드 액터의 공통 인터페이스다.
 */
class P_RD_API IBoardActorModel : public IObjectModel
{
	GENERATED_BODY()

public:
};
