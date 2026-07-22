#pragma once

/** @brief 게임플레이 연결 전 UCombatUIModel에 가짜 데이터를 밀어넣는 UI 개발용 드라이버입니다. */
// 실제 게임플레이가 붙기 전에 UI가 Set*()/Request*() 계약을 먼저 검증할 수 있게 하는 임시 대역이다.
// 게임플레이 연결 후에는 이 드라이버 대신 실제 어댑터가 같은 UIModel 계약을 사용한다.

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

#include "MockCombatDriver.generated.h"

class UCombatUIModel;

UCLASS(BlueprintType)
class P_RD_API UMockCombatDriver : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 뷰모델에 가짜 초기 상태를 채우고 입력 델리게이트를 구독한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void Start(UCombatUIModel* UIModel);

	/** @brief 가짜 예측 큐(데미지·상태이상·힐 3노드)를 만들어 밀어넣는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void PushMockPreview();

	/** @brief 큐 맨 앞 노드를 하나 재생(처리)한다. 연속 공격 재생을 흉내낸다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Mock")
	void PlayNextQueueNode();

private:
	/** @brief UI Request* 입력을 mock 상태 변경으로 흉내낸다. 실제 게임플레이 명령 처리자가 아니다. */
	UFUNCTION() void HandleCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief 월드 터치 계약이 UIModel을 통해 도달하는지만 로그로 확인한다. */
	UFUNCTION() void HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress);

private:
	/** @brief Mock이 바인딩한 UIModel. 실제 전투 수명 소유자는 아니다. */
	UPROPERTY(Transient) TObjectPtr<UCombatUIModel> mUIModel;

};
