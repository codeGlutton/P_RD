/*****************************************************************//**
 * @file   RDCollision.h
 * @brief  프로젝트 전역에서 사용하는 콜리전 프로파일/트레이스 채널 별칭 모음
 * @details
 * 실제 정의는 Config/DefaultEngine.ini의 [/Script/Engine.CollisionProfile] 섹션
 * (에디터의 Project Settings → Engine → Collision)이며,
 * 이 파일은 그 이름/슬롯을 코드에서 오타 없이 쓰기 위한 별칭만 제공한다.
 * @author 이문환
 * @date   2026-06-12
 *********************************************************************/

#pragma once

#include "RDMinimal.h"

/**
 * @brief 콜리전 프로파일 이름을 정의하는 namespace 영역
 * @details
 * 에디터의 콜리전 프리셋과 1:1 대응. SetCollisionProfileName()의 인자로 사용한다.
 */
namespace RDCollisionProfiles
{
	/** @brief 타일맵 메시 전용. 타일 선택(TileOnlyTrace)과 정보 확인(TileAnyTrace) 터치를 모두 받음 */
	inline const FName TileMap(TEXT("TileMap"));

	/** @brief 타일맵 위 액터(유닛/장애물) 전용. 정보 확인(TileAnyTrace) 터치만 받고 타일 선택은 통과 */
	inline const FName BoardActor(TEXT("TileActor"));
}

/**
 * @brief 트레이스 채널을 정의하는 namespace 영역
 * @details
 * DefaultEngine.ini의 ECC_GameTraceChannelN 슬롯과 1:1 대응.
 * GetHitResultUnderFinger() 등 트레이스 호출의 채널 인자로 사용한다.
 */
namespace RDTraceChannels
{
	/** @brief 타일맵만 맞는 채널 (조준·타일 선택·스킬 발동 등 타일 좌표 선택용) */
	constexpr ECollisionChannel TileOnlyTrace = ECC_GameTraceChannel1;

	/** @brief 타일맵+타일액터가 맞는 채널 (몬스터/타일 정보 확인용) */
	constexpr ECollisionChannel TileAnyTrace = ECC_GameTraceChannel2;
}
