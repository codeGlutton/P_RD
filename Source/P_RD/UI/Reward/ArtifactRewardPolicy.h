#pragma once

#include "RDMinimal.h"
#include "UI/Reward/RewardUITypes.h"

/**
 * 순수 보상 정책 헬퍼.
 *
 * UI가 표시한 배열은 지급 원본이 아니다. 호출자는 게임플레이의 실제
 * Room 데이터를 이 헬퍼에 넘기고, 지급 콜백 안에서 런타임 소유권 검증과
 * 실제 AddArtifact를 수행한다.
 */
namespace ArtifactRewardPolicy
{
	/** @brief 후보 풀에 요청 ID가 정확히 하나라도 포함되는지 검사한다. */
	P_RD_API bool Contains(const TArray<FPrimaryAssetId>& CandidateIds,
		const FPrimaryAssetId& RequestedId);

	/** @brief SelectOne 요청이 후보 풀에 속하면 선택 ID를 반환한다. */
	P_RD_API bool TrySelectOne(const TArray<FPrimaryAssetId>& CandidateIds,
		const FPrimaryAssetId& RequestedId,
		OUT FPrimaryAssetId& SelectedId);

	/**
	 * @brief 하나의 입력 ID를 필터링해 한 번 지급한다.
	 */
	P_RD_API FRewardGrantBundleResultUI GrantOne(
		const FPrimaryAssetId& ArtifactIds,
		TFunctionRef<bool(const FPrimaryAssetId&)> Grant);

	/**
	 * @brief 모든 입력 ID를 순서와 중복을 보존해 한 번씩 지급한다.
	 * @details 한 항목이 실패해도 후속 항목을 계속 처리한다.
	 */
	P_RD_API FRewardGrantBundleResultUI GrantAll(
		const TArray<FPrimaryAssetId>& ArtifactIds,
		TFunctionRef<bool(const FPrimaryAssetId&)> Grant);
}
