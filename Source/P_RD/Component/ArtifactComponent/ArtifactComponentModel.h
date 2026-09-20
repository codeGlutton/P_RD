/*****************************************************************//**
 * @file   ArtifactComponentModel.h
 * @brief  아티펙트를 장착/관리하고 패시브 설치/해제를 중계하는 컴포넌트 모델
 * @author 이문환
 * @date   2026-07-22
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "ArtifactComponentModel.generated.h"

class UStaticArtifactData;
class UTacticalPassive;
class UTacticalEffect;
class UPassiveComponentModel;
class UAttributeSetComponentModel;

/**
 * @brief 장착된 아티펙트와 연관된 패시브 관리
 */
USTRUCT()
struct FArtifactEntry
{
	GENERATED_BODY()

	// 장착된 아티펙트의 데이터
	UPROPERTY()
	TObjectPtr<UStaticArtifactData> mData = nullptr;

	// 아티펙트에 연관된 패시브들
	UPROPERTY()
	TArray<TObjectPtr<UTacticalPassive>> mInstalledPassives;

	// 아티펙트의 이펙트
	UPROPERTY()
	TObjectPtr<UTacticalEffect> mStatEffect = nullptr;

	// 아티펙트의 이펙트 핸들
	UPROPERTY()
	FActiveTacticalEffectHandle mStatEffectHandle;
};

/**
 * @brief 아티펙트 컴포넌트 모델
 *
 * @details
 * - UUnitModel 하위 컴포넌트
 * - 아티펙트 DA를 읽어 목록에 장착하고, 연관된 패시브를 PassiveComponentModel에 설치 및 해제
 * - 부위 개념 없으며 중복 가능하고 개수 제한 없음
 * - 원본은 UPartyModel이 소유하고 이 컴포넌트는 유닛에 적용된 결과만 추적
 */
UCLASS()
class P_RD_API UArtifactComponentModel : public UComponentModel
{
	GENERATED_BODY()

    /**
     * @brief 유닛테스트용 friend 선언
     */
	friend class FArtifactComponentModelTests;
	friend class FArtifactStatModifierTests;

public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;

	/**
	 * @brief DA를 읽어서 장착
	 */
	bool Equip(UStaticArtifactData* Data);

	/**
	 * @brief TSoftObjectPtr을 로드해서 장착
	 */
	bool Equip(const TSoftObjectPtr<UStaticArtifactData>& DataPtr);

	/**
	 * @brief 아티펙트 목록을 받아서 장착
	 */
	void EquipFrom(const TArray<TSoftObjectPtr<UStaticArtifactData>>& List);
	void EquipFrom(const TArray<FPrimaryAssetId>& List);

	/**
	 * @brief DA로 장착된 아티펙트를 해제
	 * @details 중복 장착 상태면 첫 엔트리만 해제
	 * @return 해제 성공 여부 (장착돼 있지 않으면 false)
	 */
	bool Unequip(UStaticArtifactData* Data);

	// 해당 데이터로 장착된 첫 엔트리 조회 (없으면 nullptr)
	const FArtifactEntry* GetEquipped(const UStaticArtifactData* Data) const;

	// 장착된 전체 엔트리 목록 조회
	const TArray<FArtifactEntry>& GetArtifacts() const { return mArtifacts; }

private:
	/**
	 * @brief 내부 장착 함수
	 */
	bool EquipInternal(UStaticArtifactData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp);

	/**
	 * @brief 내부 장착해제 함수
	 */
	bool UnequipInternal(UStaticArtifactData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp);

	/**
	 * @brief 인덱스로 지정해서 장착해제
	 */
	bool UnequipAt(int32 Index, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp);

	/**
	 * @brief 다른 컴포넌트 조회
	 * @note 생성 순서가 바뀔 수 있으므로 다른 컴포넌트의 존재를 가정하지 않고 조회해야 함
	 */
	UPassiveComponentModel* GetPassiveComponent() const;
	UAttributeSetComponentModel* GetAttributeComponent() const;

private:
	/**
	 * @brief 장착된 아티펙트 목록
	 * @note 중복 허용, 개수제한 없음
	 */
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Artifacts"))
	TArray<FArtifactEntry> mArtifacts;
};
