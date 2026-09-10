/*****************************************************************//**
 * @file   PartyArtifactRestoreTestsHelper.h
 * @brief  파티 아티팩트 복원 테스트용 영구데이터 정의 헤더
 * @author 이문환
 * @date   2026-09-10
 *********************************************************************/

#pragma once

#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "PartyArtifactRestoreTestsHelper.generated.h"

/**
 * @brief 파티 영구데이터 테스트용 서브클래스
 * @details protected인 아티팩트 ID 목록과 동기화 함수를 테스트에서 쓸 수 있게 노출
 */
UCLASS()
class UPartyArtifactRestoreTestData : public UPartyPersistData
{
	GENERATED_BODY()

public:
	// 저장된 아티팩트 ID 시드
	void SeedArtifact(const FPrimaryAssetId& ArtifactId)
	{
		mArtifactIds.Add(ArtifactId);
	}

	// 파티 동기화 호출 (복원 경로와 동일)
	void Sync(UPartyModel* Party, TArray<TObjectPtr<UPlayerUnitModel>>& Players)
	{
		SyncPartyPersistData(Party, Players);
	}
};
