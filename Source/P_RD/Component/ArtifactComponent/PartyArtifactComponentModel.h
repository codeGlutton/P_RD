/*****************************************************************//**
 * @file   PartyArtifactComponentModel.h
 * @brief  파티 보유 아티펙트 원본/배포 컴포넌트 모델 정의 헤더
 * @author 이문환
 * @date   2026-07-23
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "PartyArtifactComponentModel.generated.h"

class UStaticArtifactData;
class UPlayerUnitModel;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangePartyArtifactUI, const TArray<TObjectPtr<UStaticArtifactData>>& /*PartyArtifacts*/);

/**
 * @brief 파티 아티펙트 모델
 *
 * @details
 * - 파티가 보유한 아티펙트 원본(DA 목록) 관리
 * - 획득/제거 시 용병 유닛들의 ArtifactComponentModel에 장착/해제 배포
 * - 용병 목록은 소유하지 않고 호출 측(UPartyModel)에서 주입받음
 */
UCLASS()
class P_RD_API UPartyArtifactComponentModel : public UComponentModel
{
	GENERATED_BODY()

	// 유닛테스트용 friend 선언
	friend class FPartyArtifactComponentModelTests;

public:
	/**
	 * @brief 아티펙트 획득 (파티 목록에 추가하고 파티 구성원 전원에 장착)
	 * @param Data 획득할 아티펙트 데이터 (로드 완료된 포인터)
	 * @return 성공 여부 (Data가 nullptr이면 false)
	 */
	bool AddArtifact(UStaticArtifactData* Data);

	/**
	 * @brief PrimaryAssetId로 로드해서 획득
	 * @param ArtifactId 획득할 아티펙트의 PrimaryAssetId
	 * @return 성공 여부 (로드 실패 시 false)
	 */
	bool AddArtifact(const FPrimaryAssetId& ArtifactId);

	/**
	 * @brief 아티펙트 제거 (파티 목록에서 제거하고 파티 구성원 전원에서 해제)
	 * @details 중복 보유 시 하나만 제거
	 * @param Data 제거할 아티펙트 데이터
	 * @return 제거 성공 여부 (보유하지 않았으면 false)
	 */
	bool RemoveArtifact(UStaticArtifactData* Data);

	/**
	 * @brief 파티 보유 아티펙트 전체를 해당 구성원에 일괄 장착
	 * @details 구성원 합류/스폰 시 호출 (룸 진입 재스폰 포함)
	 * @param PartyMember 장착 대상 구성원 유닛
	 */
	void EquipArtifactsTo(UPlayerUnitModel* PartyMember) const;

	/**
	 * @brief 저장 데이터에서 파티 목록 복원 (이어하기)
	 * @details 장착은 수행하지 않음 (이후 구성원 스폰 시 EquipArtifactsTo가 수행)
	 * @param ArtifactIds 저장된 아티펙트 PrimaryAssetId 목록
	 */
	void RestoreFrom(const TArray<FPrimaryAssetId>& ArtifactIds);

	// 파티 보유 아티펙트 목록 조회
	const TArray<TObjectPtr<UStaticArtifactData>>& GetPartyArtifacts() const { return mPartyArtifacts; }

	// 저장용 아티펙트 PrimaryAssetId 목록 변환
	TArray<FPrimaryAssetId> GetPartyArtifactIds() const;

protected:
	TArray<TObjectPtr<UPlayerUnitModel>>& GetPartyMembers();
	const TArray<TObjectPtr<UPlayerUnitModel>>& GetPartyMembers() const;

public:
	FOnChangePartyArtifactUI OnChangePartyArtifactUI;

private:
	// 파티 보유 아티펙트 목록 (로드된 DA 참조)
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "PartyArtifacts"))
	TArray<TObjectPtr<UStaticArtifactData>> mPartyArtifacts;
};
