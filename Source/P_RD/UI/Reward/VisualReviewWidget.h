#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VisualReviewWidget.generated.h"
UCLASS(Transient)
class UVisualReviewWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual bool Initialize() override;
	virtual void NativeDestruct() override;
	UFUNCTION(BlueprintCallable) void ReplayChest();
	UFUNCTION(BlueprintCallable) void PlayAlly();
	UFUNCTION(BlueprintCallable) void PlayMonster();
	void PlayCutIn(bool Ally);
	UPROPERTY() TObjectPtr<class USkillCutInWidget> CutIn;
};
