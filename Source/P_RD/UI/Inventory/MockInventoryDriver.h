// @file MockInventoryDriver.h
// @brief 게임플레이 없이 인벤토리 화면을 선개발/검증하기 위한 가짜 데이터 공급기입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "MockInventoryDriver.generated.h"

class UInventoryUIModel;

/** @brief UIModel에 mock 런 상태 스냅샷을 채워 WBP 표시를 검증한다. */
// 실제 어댑터(URunPersistData/UUnitData 연결)가 생기기 전, UI를 먼저 만들기 위한 임시 드라이버.
UCLASS()
class P_RD_API UMockInventoryDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 가짜 보유 다이스/스킬/장비 + 메타를 만들어 UIModel에 push한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mock")
	void Start(UInventoryUIModel* UIModel);

private:
	UPROPERTY(Transient) TObjectPtr<UInventoryUIModel> mUIModel;
};
