/*****************************************************************//**
 * @file   ObstacleModel.h
 * @brief  장애물 모델 정의 헤더
 * @author 김준형
 * @date   2026-07-14
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "ObstacleModel.generated.h"

/**
 * @brief  장애물 모델
 */
UCLASS(abstract, Blueprintable)
class P_RD_API UObstacleModel : public UBoardActorModel
{
	GENERATED_BODY()

public:
	UObstacleModel();

	/* UBoardActorModel 상속 */
public:
	void PostInitializeComponentModels() override;
};
