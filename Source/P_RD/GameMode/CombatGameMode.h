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
	 * @brief 전투맵 WorldSettings에 지정된 카메라 액터로 플레이어 시점을 맞춘다.
	 *
	 * @details
	 * 카메라 위치, 회전, 투영 옵션은 코드에서 새로 만들거나 보정하지 않고,
	 * 에디터에 배치한 카메라 액터에서 직접 확인하면서 조정한다.
	 * GameMode는 WorldSettings에 지정된 액터를 ViewTarget으로 넘겨 전투 시점만 전환한다.
	 */
	void ApplyMainCameraPoint() const;
};
