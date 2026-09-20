#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/Combat/CombatUIModel.h"

#include "MercenaryHireTestsHelper.generated.h"

/** @brief 외부 텍스처나 WBP 없이 상세 겹의 수명/가시성만 검증하는 위젯. */
UCLASS()
class UMercenaryDetailTestWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	bool Initialize() override;
};

/**
 * @brief 용병 카드의 InspectUnit 요청에 상세 스냅샷을 즉시 되돌리는 테스트 수신기.
 *
 * 실제 전투에서는 ACombatGameMode가 같은 왕복 계약을 수행한다. 테스트는 전투
 * 서브시스템 전체를 띄우지 않고, HUD가 보낸 식별자와 UnitDetail 응답 이후의
 * 오버레이 상태를 한 번에 검증한다.
 */
UCLASS()
class UMercenaryDetailTestResponder : public UObject
{
	GENERATED_BODY()

public:
	void Bind(UCombatUIModel* UIModel);
	void ConfigureUnitSkillResponse(const FSkillDetailUI& SkillDetail);

	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	ECombatInputType mLastType = ECombatInputType::Cancel;
	int32 mLastPayload = INDEX_NONE;
	int32 mInspectRequestCount = 0;
	int32 mInspectSkillRequestCount = 0;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCombatUIModel> mUIModel;
	UPROPERTY(Transient)
	FSkillDetailUI mResponseSkillDetail;
};
