/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "CombatGameMode.generated.h"

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

private:
	/**
	 * @brief 전투맵 WorldSettings에 지정된 카메라 기준점으로 플레이어 시점을 맞춘다.
	 *
	 * @details
	 * MainCameraPoint는 에디터에서 방마다 배치하는 TargetPoint다.
	 * 이 값은 카메라 액터 자체가 아니라 "전투를 볼 위치와 방향"을 표시하는 기준점이므로,
	 * 전투방이 시작될 때 같은 Transform으로 CameraActor를 만들고 ViewTarget으로 지정한다.
	 */
	void ApplyMainCameraPoint() const;
};
