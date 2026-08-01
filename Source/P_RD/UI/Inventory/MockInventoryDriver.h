// @file MockInventoryDriver.h
// @brief 저장 데이터를 건드리지 않고 공용 인벤토리를 채우는 개발용 공급기입니다.
// @date 2026-06-18

#pragma once

#include "RDMinimal.h"
#include "MockInventoryDriver.generated.h"

class UInventoryUIModel;
class UTexture2D;

/** @brief transient UIModel에 골드와 아티팩트 12개를 채워 화면 밀도를 검증한다. */
UCLASS()
class P_RD_API UMockInventoryDriver : public UObject
{
	GENERATED_BODY()

public:
	UMockInventoryDriver(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** @brief 가짜 공용 골드와 아티팩트 12개를 UIModel에 push한다. */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Mock")
	void Start(UInventoryUIModel* UIModel);

private:
	UPROPERTY(Transient) TObjectPtr<UInventoryUIModel> mUIModel;

	/**
	 * @brief 개발용 filled preview 아이콘.
	 *
	 * 문자열 LoadObject만 사용하면 Android cook이 참조를 발견하지 못한다.
	 * CDO가 강하게 들고 있어 RD.InventoryPreview에서도 12종이 그대로 보인다.
	 */
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> mPreviewIcons;
};
