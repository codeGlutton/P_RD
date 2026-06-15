#pragma once

/**
 * @file CombatViewModel.h
 * @brief 전투 UI와 게임플레이를 잇는 단 하나의 경계(뷰모델)입니다.
 *
 * @details
 * - 읽기(gameplay → UI): 게임플레이/어댑터가 표시값을 Set*()으로 밀어넣고, 위젯은 Get*()으로 읽는다.
 *   진실은 게임플레이에 있고, 위젯은 절대 자기 상태/RNG를 갖지 않는다(전부 여기 캐시를 읽기만).
 * - 주기(UI → gameplay): 위젯의 탭/터치는 Request*()로 의도만 보내고, 게임플레이는 입력 델리게이트를
 *   구독해 실제 처리한다. UI는 행동을 실행하지 않는다.
 * - 큐: 행동 결과는 FCombatQueueNode 큐로 받고, 게임플레이가 한 단위 처리할 때마다 ResolveFrontQueueNode()로
 *   하나씩 비우며 OnQueueNodeResolved를 쏜다(연속 공격을 한 칸씩 재생).
 *
 * 게임플레이가 리팩토링돼도 이 클래스의 Set/Get/Request 계약만 지키면 위젯은 안 바뀐다.
 * 게임플레이 준비 전에는 Mock 드라이버가 가짜 데이터를 Set*()으로 밀어넣어 UI를 먼저 만들 수 있다.
 */

#include "RDMinimal.h"
#include "UI/Combat/CombatViewTypes.h"

#include "CombatViewModel.generated.h"

/** @brief UI가 게임플레이에 보내는 의도 종류(index 기반 명령). 타일/월드 터치는 별도 델리게이트로 보낸다. */
UENUM(BlueprintType)
enum class ECombatInputType : uint8
{
	SelectSkill,      // payload = SkillIndex
	ToggleDice,       // payload = DiceIndex
	RollDice,         // payload 없음(터치로 굴림)
	LongPressSkill,   // payload = SkillIndex (상세창)
	LongPressUnit,    // payload = UnitId (적 정보)
	Move,             // payload 없음(채운 무브포인트 소모)
	EndTurn,          // payload 없음
	Cancel            // payload 없음(딴 데 탭 = 초기화)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatViewChanged, ECombatViewDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatQueueNodeResolved, FCombatQueueNode, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatCommand, ECombatInputType, Type, int32, IntPayload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatWorldTouch, FVector2D, ScreenPosition, bool, bLongPress);

/**
 * @brief 전투 조작 UI의 뷰모델. PlayerController나 전투 HUD가 하나 소유해 위젯들이 공유한다.
 */
UCLASS(BlueprintType)
class P_RD_API UCombatViewModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 어떤 도메인이 갱신됐는지 알림. 위젯은 자기 도메인만 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatViewChanged OnViewChanged;

	/** @brief 큐 노드 하나가 처리(재생)됐음을 알림. 위젯은 머리 위 숫자 등을 띄우고 한 칸 비운다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatQueueNodeResolved OnQueueNodeResolved;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnCombatCommand OnCombatCommand;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnCombatWorldTouch OnCombatWorldTouch;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestSelectSkill(int32 SkillIndex);
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestToggleDice(int32 DiceIndex);
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestRollDice();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressSkill(int32 SkillIndex);
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressUnit(int32 UnitId);
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestMove();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestEndTurn();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestCancel();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitViews(const TArray<FUnitView>& Units);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitDetail(const FUnitDetailView& Detail);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetDiceViews(const TArray<FDiceSlotView>& Dice);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillViews(const TArray<FSkillView>& Skills);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillDetail(const FSkillDetailView& Detail);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTileStates(const TArray<FTileViewState>& Tiles);
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTurnView(const FTurnView& Turn);

	/** @brief 행동/예측 결과 큐를 통째로 설정(예측 표시용). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetActionQueue(const TArray<FCombatQueueNode>& Queue);

	/** @brief 큐 맨 앞 노드 하나를 처리 완료로 비우고 OnQueueNodeResolved를 쏜다(애니 한 단위 끝날 때 호출). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void ResolveFrontQueueNode();

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FUnitView>& GetUnitViews() const { return mUnitViews; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FUnitDetailView& GetUnitDetail() const { return mUnitDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FDiceSlotView>& GetDiceViews() const { return mDiceViews; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<int32>& GetSelectedDiceIndices() const { return mSelectedDiceIndices; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 GetSelectedDiceSum() const { return mSelectedDiceSum; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FSkillView>& GetSkillViews() const { return mSkillViews; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FSkillDetailView& GetSkillDetail() const { return mSkillDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FTileViewState>& GetTileStates() const { return mTileStates; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FCombatQueueNode>& GetActionQueue() const { return mActionQueue; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FTurnView& GetTurnView() const { return mTurnView; }

private:
	UPROPERTY(Transient) TArray<FUnitView> mUnitViews;
	UPROPERTY(Transient) FUnitDetailView mUnitDetail;
	UPROPERTY(Transient) TArray<FDiceSlotView> mDiceViews;
	UPROPERTY(Transient) TArray<int32> mSelectedDiceIndices;
	UPROPERTY(Transient) int32 mSelectedDiceSum = 0;
	UPROPERTY(Transient) TArray<FSkillView> mSkillViews;
	UPROPERTY(Transient) FSkillDetailView mSkillDetail;
	UPROPERTY(Transient) TArray<FTileViewState> mTileStates;
	UPROPERTY(Transient) TArray<FCombatQueueNode> mActionQueue;
	UPROPERTY(Transient) FTurnView mTurnView;
};
