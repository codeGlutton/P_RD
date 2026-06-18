/*****************************************************************//**
 * @file   BoardActorModel.h
 * @brief  보드에 올라가는 액터 데이터 모델 클래스 정의 헤더
 * @author 이문환
 * @date   2026-06-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "ObjectModel.h"
#include "BoardActorModel.generated.h"

/**
 * @brief  보드에 올라가는 액터 데이터 모델 클래스
 * @details 보드 위에 올라가는 액터(플레이어, 몬스터 등)의 데이터 모델 베이스 클래스다.
 */
UCLASS()
class P_RD_API UBoardActorModel : public UObject, public IObjectModel
{
	GENERATED_BODY()

public:
};
