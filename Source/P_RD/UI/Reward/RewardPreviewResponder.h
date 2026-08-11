#pragma once

/** @brief 개발 프리뷰에서 보상 요청을 즉시 성공 처리하는 임시 응답기입니다. */

#include "RDMinimal.h"
#include "UI/Reward/RewardUITypes.h"

#include "RewardPreviewResponder.generated.h"

class URewardUIModel;

UCLASS()
class P_RD_API URewardPreviewResponder : public UObject
{
	GENERATED_BODY()

public:
	void Bind(URewardUIModel* UIModel);
	void SetOnPreviewClosed(FSimpleDelegate InCallback);

private:
	UFUNCTION() void HandleClaimed();
	UFUNCTION() void HandleClaimRequested(ERewardClaimKind ClaimKind, int32 ChoiceIndex);
	void Unbind();

	UPROPERTY(Transient) TObjectPtr<URewardUIModel> mUIModel;
	FSimpleDelegate mOnPreviewClosed;
};
