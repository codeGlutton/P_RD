// @file InventoryUIModel.h
// @brief 인벤토리(런 상태 확인) 화면 UI와 게임플레이를 잇는 경계(뷰모델)입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "UI/Inventory/InventoryUITypes.h"
#include "InventoryUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUIChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemLongPress, EInventoryItemKind, Kind, int32, ItemIndex);

/** @brief 인벤토리 화면 뷰모델. 런 상태 패널을 열 때 게임플레이가 하나 만들어 위젯에 물린다. */
// RewardUIModel과 같은 계약: 게임플레이가 SetInventory()로 밀어넣고, 위젯은 GetInventory()로 읽는다.
// 읽기 전용 화면이라 입력 의도는 항목 롱프레스(상세 보기) 하나뿐이다. UIModel은 데이터를 만들지 않는다.
UCLASS(BlueprintType)
class P_RD_API UInventoryUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 인벤토리 표시값이 설정/갱신됐음을 알림. 위젯은 목록을 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|UI")
	FOnInventoryUIChanged OnUIChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 항목을 길게 눌렀음(상세 보기 요청). */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Input")
	FOnInventoryItemLongPress OnItemLongPress;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief 항목 롱프레스 의도를 게임플레이 구독자에게 전달한다(상세 패널은 게임플레이가 띄운다). */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Input") void RequestItemLongPress(EInventoryItemKind Kind, int32 ItemIndex);

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이가 모은 런 상태 표시 스냅샷을 저장하고 변경 알림을 보낸다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Push") void SetInventory(const FInventoryUI& Inventory);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 위젯이 현재 인벤토리 스냅샷을 읽는다; 반환 참조는 다음 SetInventory()까지 유효하다. */
	UFUNCTION(BlueprintPure, Category = "Inventory|Read") const FInventoryUI& GetInventory() const { return mInventory; }

private:
	/** @brief 마지막으로 Push된 인벤토리 표시값; Transient라 세이브/에셋 상태로 남기지 않는다. */
	UPROPERTY(Transient) FInventoryUI mInventory;
};
