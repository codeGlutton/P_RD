/*****************************************************************//**
 * @file   PartyArtifactTestsHelper.h
 * @brief  파티 아티펙트 테스트용 Mock 정의 헤더
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Pawn/Player/PlayerUnitModel.h"
#include "PartyArtifactTestsHelper.generated.h"

/**
 * @brief 파티 구성원 Mock
 * @details abstract인 UPlayerUnitModel을 테스트에서 인스턴스화하기 위한 구체 서브클래스.
 * 필요한 컴포넌트는 부모 생성자가 만들고, PostInitProperties가 자동 등록함.
 */
UCLASS()
class UMockPartyMemberModel : public UPlayerUnitModel
{
	GENERATED_BODY()
};
