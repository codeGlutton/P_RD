#pragma once

/** @brief 전투 UI와 게임플레이를 잇는 단일 뷰모델 계약입니다. */
// gameplay -> UI: 게임플레이/어댑터가 Set*()으로 표시값을 밀어넣고, 위젯은 Get*()으로 읽는다.
// UI -> gameplay: 위젯은 Request*()로 의도만 보내고, 게임플레이가 입력 델리게이트를 구독해 실제 처리한다.
// Queue: 행동 결과는 FCombatQueueNode 큐로 받고, ResolveFrontQueueNode() 한 번에 한 연출 단위씩 비운다.
// 계약 의도: 게임플레이가 리팩토링돼도 Set/Get/Request 경계가 유지되면 위젯 변경을 최소화한다.

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"
#include "UI/Reward/RewardUITypes.h"

#include "CombatUIModel.generated.h"

/** @brief UI가 게임플레이에 보내는 의도 종류(index 기반 명령). 타일/월드 터치는 별도 델리게이트로 보낸다. */
UENUM(BlueprintType)
enum class ECombatInputType : uint8
{
	SelectSkill,      // payload = SkillIndex
	LongPressSkill,   // payload = SkillIndex (상세창)
	LongPressUnit,    // payload = UnitId (적 정보)
	Move,             // payload 없음(채운 무브포인트 소모)
	EndTurn,          // payload 없음
	Cancel,           // payload 없음(딴 데 탭 = 초기화)
	LongPressEquip,   // payload = SlotIndex (장비 상세)
	InspectUnit,      // payload = UnitId (그 유닛의 스킬을 본다)
	Confirm,          // payload 없음(겨냥한 칸을 그대로 확정)
	// payload = SkillIndex. **지금 상세창에 뜬 유닛의** 스킬 상세를 청한다.
	// LongPressSkill과 다른 것은 기준 유닛이다 -- 그쪽은 카드 레일(조종 중인 아군),
	// 이쪽은 길게 눌러 들여다보는 중인 유닛이라 적 스킬일 수도 있다.
	InspectUnitSkill
};

class UTexture2D;
struct FPresentationBarrier;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatUIChanged, ECombatUIDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatQueueNodeResolved, FCombatQueueNode, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatActionResolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatCommand, ECombatInputType, Type, int32, IntPayload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatWorldTouch, FVector2D, ScreenPosition, bool, bLongPress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatFloatingLog, FCombatFloatingLogRequest, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatFloatingLogMotionFinished, int32, MotionIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatFloatingLogsCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatResultOpenRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbandonRun);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRetryCombat);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBeginCombatPresentation, TSharedPtr<FPresentationBarrier> /*Barrier*/)

/** @brief 전투 조작 UI의 뷰모델. PlayerController나 전투 HUD가 하나 소유해 위젯들이 공유한다. */
UCLASS(BlueprintType)
class P_RD_API UCombatUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/** @brief 어떤 도메인이 갱신됐는지 알림. 위젯은 자기 도메인만 다시 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatUIChanged OnUIChanged;

	/** @brief 큐 노드 하나가 처리(재생)됐음을 알림. 위젯은 머리 위 숫자 등을 띄우고 한 칸 비운다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatQueueNodeResolved OnQueueNodeResolved;

	/** @brief 스킬/액션이 확정·취소되어 빌드가 끝났음을 알림. 위젯은 스킬 선택 강조를 푼다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatActionResolved OnActionResolved;

	/** @brief 전투 이벤트(HP 증감 등)를 지정 월드 위치에 플로팅 텍스트로 띄우라는 알림. HUD가 순차 큐로 재생한다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLog OnCombatFloatingLog;

	/** @brief 애니메이션 모션 하나가 끝났으니 해당 MotionIndex의 플로팅 로그를 정리하라는 알림. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLogMotionFinished OnCombatFloatingLogMotionFinished;

	/** @brief 현재 떠 있는(그리고 대기 중인) 플로팅 로그를 전부 지우라는 알림. 시뮬레이션 전환/취소로 미리보기 목록을 통째로 버릴 때 쓴다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLogsCleared OnCombatFloatingLogsCleared;

	/** @brief 전투 보상 오버레이를 열라는 알림 */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatResultOpenRequested OnCombatResultOpenRequested;

public:
	FOnBeginCombatPresentation OnBeginCombat;
	FOnBeginCombatPresentation OnEndCombat;
	FOnBeginCombatPresentation OnBeginAnyTurn;
	FOnBeginCombatPresentation OnBeginAnyRound;
	FOnBeginCombatPresentation OnEndAnyTurn;
	FOnBeginCombatPresentation OnBeginAnyTurnAction;
	FOnBeginCombatPresentation OnEndAnyTurnAction;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
	// UI는 Request*()로 의도만 보낸다. 게임플레이가 아래 델리게이트를 구독해 실제 처리해야 한다.
public:
	/** @brief UI 명령(스킬선택/이동/턴종료/취소/롱프레스). */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnCombatCommand OnCombatCommand;

	/** @brief 월드 터치(UI는 스크린좌표만 전달). [게임플레이 구독] 스크린→타일 변환은 게임플레이가 수행. [합의필요] 변환 책임 일원화. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnCombatWorldTouch OnCombatWorldTouch;

	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnAbandonRun OnAbandonRun;

	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnRetryCombat OnRetryCombat;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief SkillIndex를 그대로 게임플레이에 전달한다. UI는 스킬 객체를 직접 들고 있지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestSelectSkill(int32 SkillIndex);
	/** @brief SkillIndex 상세 표시 요청을 보낸다. 상세 데이터는 SetSkillDetail()로 되돌아온다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressSkill(int32 SkillIndex);
	/** @brief UnitId 상세 표시 요청을 보낸다. UI는 UnitId를 상태 객체로 해석하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressUnit(int32 UnitId);
	/** @brief MOVE 모드 진입 의도만 보낸다. 실제 타일 판정은 월드 터치 입력 뒤 처리된다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestMove();
	/** @brief 턴 종료 버튼 의도. 실제 턴 시스템 호출과 실패 처리는 게임플레이가 맡는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestEndTurn();
	/**
	 * @brief 겨냥해 둔 칸을 확정한다.
	 *
	 * 판에서 그 칸을 다시 누르는 것과 같은 뜻이다. 화면 아래 단추로도 할 수
	 * 있어야 해서 따로 둔다 -- 좁은 화면에서 칸을 두 번 정확히 짚는 것은
	 * 손가락으로 하기 어렵다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Request") void RequestConfirm();

	/** @brief 현재 스킬/타겟 선택 취소 의도. UI 강조 해제는 OnActionResolved로 되돌아온다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestCancel();

	/**
	 * @brief 이 유닛의 스킬을 보여 달라.
	 *
	 * @details
	 * 하단 용병 칸을 누르면 그 용병이 무엇을 할 수 있는지 보고 싶다는 뜻이다.
	 * 제 차례가 아니어도 보여 준다 -- 다만 그때는 카드가 전부 꺼진 채로 온다.
	 * 무엇을 들고 있는지 아는 것과 지금 쓸 수 있는 것은 다른 이야기다.
	 *
	 * 차례가 넘어가면 저절로 풀린다. 화면이 따로 되돌릴 필요가 없다.
	 * @param UnitId 볼 유닛. INDEX_NONE 이면 지금 차례인 유닛으로 되돌린다
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestInspectUnit(int32 UnitId);
	/** @brief 장비 슬롯 상세 요청. SlotIndex는 FEquipmentUI.mSlotIndex와 같은 계약이다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressEquip(int32 SlotIndex);

	/**
	 * @brief 상세창에 뜬 유닛의 스킬 하나를 보여 달라.
	 *
	 * @details
	 * 유닛 상세창의 스킬 칸을 탭했다는 뜻이다. 어느 유닛인지는 보내지 않는다 --
	 * 그 상세를 내려 준 게임플레이가 이미 알고 있고, 화면이 유닛을 다시 짚으면
	 * 기준이 두 곳에 생긴다.
	 * @param SkillIndex FUnitDetailSkillUI.mSkillIndex 를 그대로 되돌려 보낸다
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestInspectUnitSkill(int32 SkillIndex);
	/** @brief 화면 좌표와 롱프레스 여부만 넘긴다. 월드/타일 변환은 UIModel 바깥의 책임이다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestAbandonRun();
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestRetryCombat();

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ─────────
	   각 Set*()은 UI가 그리려면 게임플레이가 반드시 공급해야 하는 값이다(UI는 못 만듦).
	   [소스]=가져올 곳(정해짐), [합의필요]=진짜 소스 미정(현재 Mock/placeholder). */
public:
	/** @brief 전투 결과 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetCombatResultUI(const FCombatResultUI& Result);
	/** @brief 전체 유닛 HP/이동력/타일/HP바위치/상태태그. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitUIs(const TArray<FUnitUI>& Units);
	/** @brief 유닛 롱프레스 상세(이름/레벨/초상화/패시브). [합의필요] UUnitData 연결. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitDetail(const FUnitDetailUI& Detail);

	/**
	 * @brief 지금 겨냥한 자리를 내린다.
	 *
	 * @details
	 * 판을 톡 친 좌표(RequestWorldTouch)를 받아 게임플레이가 어느 타일인지 풀고
	 * 이 함수로 내려준다. 화면은 타일맵 좌표계를 모르므로 스스로 못 만든다.
	 *
	 * 이 값이 바뀌면 그 자리에 쓸 수 있는 스킬이 달라진다. 같이 SetSkillUIs()
	 * 로 mIsUsable 을 다시 내려 주어야 카드가 맞게 켜지고 꺼진다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTarget(const FCombatTargetUI& Target);
	/** @brief 스킬 레일(이름/아이콘/사용가능). [합의필요] 소스=USkillComponent(김준형), 현재 Mock. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillUIs(const TArray<FSkillUI>& Skills);

	/** @brief 선택한 스킬 index. [소스] SRPGSkillBuildAction.mSelectedSkill. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSelectedSkill(int32 SelectedIndex);
	/** @brief 스킬 롱프레스 상세. [합의필요] 스킬 데이터 연결. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillDetail(const FSkillDetailUI& Detail);
	/** @brief 턴유닛/라운드/페이즈/턴순서. mPhase=ECombatBuildPhaseUI(UI 전용). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTurnUI(const FTurnUI& Turn);
	/** @brief 조작 빌드 페이즈만 갱신(스킬/이동 빌드 공용). 턴 스냅샷 전체 교체 없이 페이즈 전환을 알릴 때 사용. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetBuildPhase(ECombatBuildPhaseUI Phase);
	/** @brief 확정 전 행동 종류와 AP 예정 소모량을 갱신한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetPendingAction(const FCombatPendingActionUI& PendingAction);
	/**
	 * @brief 이동 연출의 완료 칸 수를 표시 AP에 반영한다.
	 * @details 타일 효과가 반영된 현재 실제 AP에서 절대 완료 칸 수를 빼서 계산한다.
	 *          실제 FUnitUI 스냅샷은 수정하지 않는다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetMoveAPStepPresentation(int32 UnitId, int32 CompletedStepCount);
	/** @brief 이동 연출용 AP 표시값을 해제한다. UnitId가 INDEX_NONE이면 대상을 가리지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void ClearMoveAPStepPresentation(int32 UnitId);
	/** @brief 장비 슬롯(아이콘/이름/장착/희귀도). [합의필요] 장비 데이터 소스 미정, 현재 임시. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetEquipmentUIs(const TArray<FEquipmentUI>& Equipment);
	/** @brief 장비 롱프레스 상세 스냅샷을 교체한다(GameMode의 PushEquipmentDetailUIData가 채워 밀어넣는다). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetEquipmentDetail(const FEquipmentDetailUI& Detail);
	/** @brief 상단 메타(Gold/Lv/Exp). [합의필요] 진짜소스=UUnitData/URunPersistData, 현재 placeholder. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetPlayerMeta(const FPlayerMetaUI& Meta);

	/** @brief 행동/예측 결과 큐를 통째로 설정(예측 표시용). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetActionQueue(const TArray<FCombatQueueNode>& Queue);

	/** @brief 큐 맨 앞 노드 하나를 처리 완료로 비우고 OnQueueNodeResolved를 쏜다(애니 한 단위 끝날 때 호출). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void ResolveFrontQueueNode();

	/** @brief 스킬/액션 빌드가 끝났음을 UI에 알린다(OnActionResolved). 게임플레이가 확정/취소 시 호출. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyActionResolved();

	/** @brief 월드 위치 기준 플로팅 로그 한 건을 요청한다. HUD가 IconType/ColorType을 실제 표현으로 변환한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLog(const FCombatFloatingLogRequest& Request);

	/** @brief 여러 플로팅 로그를 한 번에 요청한다. Sequence 기준으로 브로드캐스트하면 HUD가 수신 순서대로 순차 재생한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLogs(const TArray<FCombatFloatingLogRequest>& Requests);

	/** @brief 같은 액션의 MotionEventLogs 배열에서 MotionIndex번째 모션 연출이 끝났음을 알린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLogMotionFinished(int32 MotionIndex);

	/** @brief 현재/대기 중인 플로팅 로그를 전부 지운다. 시뮬레이션이 다른 것으로 넘어가 미리보기 목록을 통째로 버릴 때 호출. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLogsCleared();

	/** @brief 전투 보상 오버레이 열기를 요청한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatResultOpenRequested();

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FCombatResultUI& GetCombatResultUI() const { return mCombatResultUI; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FUnitUI>& GetUnitUIs() const { return mUnitUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FUnitDetailUI& GetUnitDetail() const { return mUnitDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FSkillUI>& GetSkillUIs() const { return mSkillUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 GetSelectedSkillIndex() const { return mSelectedSkillIndex; }

	/** @brief 찜해 둔 대상. 없으면 INDEX_NONE. */
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FCombatTargetUI& GetTarget() const { return mTarget; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FSkillDetailUI& GetSkillDetail() const { return mSkillDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FCombatQueueNode>& GetActionQueue() const { return mActionQueue; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FTurnUI& GetTurnUI() const { return mTurnUI; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FCombatPendingActionUI& GetPendingAction() const { return mPendingAction; }
	/** @brief 실제 AP 또는 현재 이동 연출 단계에 맞춘 표시 AP를 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 ResolveDisplayedMovementPoint(const FUnitUI& Unit) const;
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FEquipmentUI>& GetEquipmentUIs() const { return mEquipmentUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FEquipmentDetailUI& GetEquipmentDetail() const { return mEquipmentDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FPlayerMetaUI& GetPlayerMeta() const { return mPlayerMeta; }

private:
	/** @brief 마지막으로 push된 전투 결과 */
	UPROPERTY(Transient) FCombatResultUI mCombatResultUI;
	/** @brief 마지막으로 push된 유닛 표시 스냅샷. 위젯은 참조로 읽고 수정하지 않는다. */
	UPROPERTY(Transient) TArray<FUnitUI> mUnitUIs;
	/** @brief 마지막 유닛 상세 스냅샷. 롱프레스 상세 패널의 단일 소스다. */
	UPROPERTY(Transient) FUnitDetailUI mUnitDetail;
	/** @brief 스킬 레일 표시 스냅샷. SkillIndex payload와 같은 index 계약을 가진다. */
	UPROPERTY(Transient) TArray<FSkillUI> mSkillUIs;
	/** @brief 현재 선택한 스킬 index. index는 mSkillUIs 배열 기준이다. */
	UPROPERTY(Transient) int32 mSelectedSkillIndex = 0;

	/** @brief 찜해 둔 대상 유닛. 없으면 INDEX_NONE. */
	UPROPERTY(Transient) FCombatTargetUI mTarget;
	/** @brief 마지막 스킬 상세 스냅샷. */
	UPROPERTY(Transient) FSkillDetailUI mSkillDetail;
	/** @brief 아직 재생되지 않은 행동 결과 큐. ResolveFrontQueueNode()가 앞에서 하나씩 제거한다. */
	UPROPERTY(Transient) TArray<FCombatQueueNode> mActionQueue;
	/** @brief 현재 턴/라운드/페이즈 표시 스냅샷. */
	UPROPERTY(Transient) FTurnUI mTurnUI;
	/** @brief 현재 선택 중인 이동 경로 또는 스킬의 AP 예정 소모. */
	UPROPERTY(Transient) FCombatPendingActionUI mPendingAction;
	/** @brief 타일별 이동 연출 중 표시 AP를 덮어쓸 유닛. 비활성은 INDEX_NONE. */
	UPROPERTY(Transient) int32 mMoveAPPresentationUnitId = INDEX_NONE;
	/** @brief 현재 완료 칸 수까지 반영한 화면용 AP. */
	UPROPERTY(Transient) int32 mMoveAPPresentationDisplayed = 0;
	/** @brief 현재까지 물리적으로 도착한 타일 수. */
	UPROPERTY(Transient) int32 mMoveAPPresentationCompletedSteps = 0;
	/** @brief 장비 슬롯 표시 스냅샷. */
	UPROPERTY(Transient) TArray<FEquipmentUI> mEquipmentUIs;
	UPROPERTY(Transient) FEquipmentDetailUI mEquipmentDetail;
	/** @brief 플레이어 메타 표시 스냅샷. */
	UPROPERTY(Transient) FPlayerMetaUI mPlayerMeta;
};
