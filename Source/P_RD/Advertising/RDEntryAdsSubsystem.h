#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Advertising/EntryAdGate.h"
#include "RDEntryAdsSubsystem.generated.h"

/** Beta integration using Google's demo interstitial only; production placement requires a separate review. */
UCLASS()
class P_RD_API URDEntryAdsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;
	static bool IsEnabled();
	void Prepare();
	void BeforeEntry(FSimpleDelegate Continuation);
	void CancelEntry();
	void FinishEntry(int32 Ticket);
private:
	FEntryAdGate EntryGate;
};
