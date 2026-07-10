/*****************************************************************//**
 * @file   EquipmentComponentModel.h
 * @brief  장비를 장착·관리하고 패시브 설치/해제를 중계하는 컴포넌트 모델
 * @author 이문환
 * @date   2026-06-28
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Component/ComponentModel.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "TAS/Effect/ActiveTacticalEffect.h"
#include "EquipmentComponentModel.generated.h"

class UStaticEquipmentData;
class UTacticalPassive;
class UTacticalEffect;
class UPassiveComponentModel;
class UAttributeSetComponentModel;

/**
 * @brief 한 슬롯에 장착된 장비와 그로 인해 설치된 런타임 객체 추적
 */
USTRUCT()
struct FEquippedEntry
{
	GENERATED_BODY()

	// 장착된 장비 데이터
	UPROPERTY()
	TObjectPtr<UStaticEquipmentData> mData = nullptr;

	// 이 장비가 설치한 패시브 인스턴스들 (해제 시 RemovePassive 대상)
	UPROPERTY()
	TArray<TObjectPtr<UTacticalPassive>> mInstalledPassives;

	// 장비 고유 스탯을 적용하는 런타임 이펙트 인스턴스 (엔트리 수명 동안 GC 보호)
	UPROPERTY()
	TObjectPtr<UTacticalEffect> mStatEffect = nullptr;

	// 적용된 장비 스탯 이펙트 핸들 (해제 시 제거용)
	UPROPERTY()
	FActiveTacticalEffectHandle mStatEffectHandle;
};

/**
 * @brief 장비 보유·장착 컴포넌트 모델
 *
 * @details
 * UUnitModel 하위 컴포넌트. 장비 DA를 읽어 슬롯(EEquipmentType)에 장착하고,
 * 장비가 참조하는 패시브를 형제 PassiveComponentModel에 설치/해제한다.
 * 슬롯당 1개만 장착하며, 같은 슬롯에 다시 장착하면 기존 장비를 먼저 해제한다.
 *
 * @par 부착
 * 액터 모델 생성자에서 CreateDefaultSubobject로 등록, FindComponentModelByClass로 조회.
 *
 * @par 형제 컴포넌트 접근
 * 초기화 순서를 가정하지 않도록 캐시하지 않고 매번 오너에서 지연 조회.
 */
UCLASS()
class P_RD_API UEquipmentComponentModel : public UComponentModel
{
	GENERATED_BODY()

	// 자동화 테스트가 오너 없이 형제 컴포넌트를 직접 주입해 내부 로직을 검증하기 위한 friend
	friend class FEquipmentComponentModelTests;
	// 장비 스탯 적용/원복(라이브 월드 기반) 테스트용 friend
	friend class FEquipmentStatModifierTests;

public:
	virtual void Initialize() override;
	virtual void Uninitialize() override;

public:
	/**
	 * @brief 장비를 슬롯에 장착 (기존 점유 시 먼저 해제)
	 * @param Data 장착할 장비 데이터 (로드 완료된 포인터)
	 * @return 장착 성공 여부
	 */
	bool Equip(UStaticEquipmentData* Data);

	/**
	 * @brief 소프트 참조를 로드해 장착
	 */
	bool Equip(const TSoftObjectPtr<UStaticEquipmentData>& DataPtr);

	/**
	 * @brief 장비 목록(스폰 데이터 등)을 일괄 장착
	 */
	void EquipFrom(const TArray<TSoftObjectPtr<UStaticEquipmentData>>& List);
	void EquipFrom(const TArray<FPrimaryAssetId>& List);

	/**
	 * @brief 슬롯의 장비를 해제 (설치한 패시브·스탯 제거)
	 * @return 해제 성공 여부 (장착돼 있지 않으면 false)
	 */
	bool Unequip(EEquipmentType Slot);

	// 슬롯에 장착된 엔트리 조회 (없으면 nullptr)
	const FEquippedEntry* GetEquipped(EEquipmentType Slot) const;

private:
	/**
	 * @brief 장착 코어 로직 (형제 컴포넌트를 인자로 받아 오너 조회와 분리)
	 * @details 공개 Equip은 오너에서 형제를 찾아 이 함수를 호출. 테스트는 직접 주입해 호출.
	 */
	bool EquipInternal(UStaticEquipmentData* Data, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp);

	/**
	 * @brief 해제 코어 로직 (형제 컴포넌트를 인자로 받아 오너 조회와 분리)
	 */
	bool UnequipInternal(EEquipmentType Slot, UPassiveComponentModel* PassiveComp, UAttributeSetComponentModel* AttrComp);

	// 형제 컴포넌트 지연 조회 (init 순서 무가정, 캐시 금지)
	UPassiveComponentModel* GetPassiveComponent() const;
	UAttributeSetComponentModel* GetAttributeComponent() const;

private:
	// 슬롯별 장착 엔트리 (슬롯당 1개)
	UPROPERTY(VisibleAnywhere, meta = (DisplayName = "Equipped"))
	TMap<EEquipmentType, FEquippedEntry> mEquipped;
};
