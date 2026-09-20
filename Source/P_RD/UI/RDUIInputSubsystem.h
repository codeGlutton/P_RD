#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RDUIInputSubsystem.generated.h"

class UUserWidget;
class IInputProcessor;
struct FKey;

/** Routes system Back to the actual highest visible layer, independently of keyboard focus. */
UCLASS()
class P_RD_API URDUIInputSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	void Register(UUserWidget* Owner, TFunction<UUserWidget*()> Layer, TFunction<bool()> Handler);
	void Unregister(UUserWidget* Owner);
	bool RouteBack();
	static bool IsBackKey(const FKey& Key);
private:
	struct FEntry
	{
		TWeakObjectPtr<UUserWidget> Owner;
		TFunction<UUserWidget*()> Layer;
		TFunction<bool()> Handler;
	};
	TArray<FEntry> Entries;
	TSharedPtr<IInputProcessor> Processor;
};
