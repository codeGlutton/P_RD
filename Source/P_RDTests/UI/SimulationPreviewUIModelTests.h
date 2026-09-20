#pragma once

#include "CoreMinimal.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/SimulationPreviewUIModel.h"

#include "SimulationPreviewUIModelTests.generated.h"

UCLASS()
class USimulationPreviewUIModelTestListener : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandlePreviewChanged(ESimulationPreviewDomainUI Domain);
	UFUNCTION()
	void HandlePreviewEventBatch(FCombatEventBatchUI Batch);
	UFUNCTION()
	void HandlePreviewCleared();
	UFUNCTION()
	void HandleLiveEventBatch(FCombatEventBatchUI Batch);

	int32 mPreviewChangedCallCount = 0;
	ESimulationPreviewDomainUI mLastPreviewDomain = ESimulationPreviewDomainUI::All;
	int32 mPreviewBatchCallCount = 0;
	FCombatEventBatchUI mLastPreviewBatch;
	int32 mPreviewClearedCallCount = 0;
	int32 mLiveBatchCallCount = 0;
	FCombatEventBatchUI mLastLiveBatch;
};
