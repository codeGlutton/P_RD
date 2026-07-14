#pragma once

/** @brief 전투 UI와 게임플레이를 잇는 단일 뷰모델 계약입니다. */
// gameplay -> UI: 게임플레이/어댑터가 Set*()으로 표시값을 밀어넣고, 위젯은 Get*()으로 읽는다.
// UI -> gameplay: 위젯은 Request*()로 의도만 보내고, 게임플레이가 입력 델리게이트를 구독해 실제 처리한다.
// Queue: 행동 결과는 FCombatQueueNode 큐로 받고, ResolveFrontQueueNode() 한 번에 한 연출 단위씩 비운다.
// 계약 의도: 게임플레이가 리팩토링돼도 Set/Get/Request 경계가 유지되면 위젯 변경을 최소화한다.

#include "RDMinimal.h"
#include "UI/Combat/CombatUITypes.h"

#include "CombatUIModel.generated.h"

/** @brief UI가 게임플레이에 보내는 의도 종류(index 기반 명령). 타일/월드 터치는 별도 델리게이트로 보낸다. */
UENUM(BlueprintType)
enum class ECombatInputType : uint8
{
	SelectSkill = 0,      // payload = SkillIndex
	ToggleDice = 1,       // payload = DiceIndex
	RollDice = 2,         // payload 없음(터치로 굴림)
	LongPressSkill = 3,   // payload = SkillIndex (상세창)
	Move = 5,             // payload 없음(채운 무브포인트 소모). 4는 제거된 legacy 유닛 입력 값
	EndTurn = 6,          // payload 없음
	Cancel = 7,           // payload 없음(딴 데 탭 = 초기화)
	LongPressEquip = 8,   // payload = SlotIndex (장비 상세)
	Confirm = 9           // payload 없음(현재 스킬/이동 빌드 확정)
};

class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatUIChanged, ECombatUIDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatQueueNodeResolved, FCombatQueueNode, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatActionResolved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatCommand, ECombatInputType, Type, int32, IntPayload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApplyDiceResults, const TArray<int32>&, RolledFaceIndices);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatFloatingLog, FCombatFloatingLogRequest, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCombatFloatingLogMotionFinished, int32, TurnIndex, int32, ActionIndex, int32, MotionIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatFloatingLogsCleared);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatDiceRollRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatUnitDetailReady, FUnitDetailUI, Detail);

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

	/** @brief 스킬/액션이 확정·취소되어 빌드가 끝났음을 알림. 위젯은 스킬/주사위 선택 강조를 푼다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatActionResolved OnActionResolved;

	/** @brief 전투 이벤트(HP 증감 등)를 지정 월드 위치에 플로팅 텍스트로 띄우라는 알림. HUD가 순차 큐로 재생한다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLog OnCombatFloatingLog;

	/** @brief 애니메이션 모션 하나가 끝났으니 해당 (TurnIndex, ActionIndex, MotionIndex)에 묶인 플로팅 로그를 정리하라는 알림. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLogMotionFinished OnCombatFloatingLogMotionFinished;

	/** @brief 현재 떠 있는(그리고 대기 중인) 플로팅 로그를 전부 지우라는 알림. 시뮬레이션 전환/취소로 미리보기 목록을 통째로 버릴 때 쓴다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatFloatingLogsCleared OnCombatFloatingLogsCleared;

	/** @brief 턴 시작 주사위 굴림 오버레이를 열라는 알림(프레임워크 OnShowDicePanelAnyTurnUI 중계). 첫 턴 이후에도 굴림 UI가 자동으로 뜨게 한다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatDiceRollRequested OnDiceRollRequested;

	/** @brief GameMode가 월드 대상을 판정해 유닛 상세 DTO를 완성했음을 알린다. HUD는 이 값으로 상세 패널만 그린다. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|View")
	FOnCombatUnitDetailReady OnUnitDetailReady;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
	// UI는 Request*()로 의도만 보낸다. 게임플레이가 아래 델리게이트를 구독해 실제 처리해야 한다.
public:
	/** @brief UI 명령(스킬선택/주사위토글/굴림/이동/턴종료/취소/롱프레스). [게임플레이 구독] RollDice면 굴려서 SetDiceUIs로 결과 push. */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnCombatCommand OnCombatCommand;

	/** @brief 입장 물리 굴림의 결과면(0-base index)을 전투 풀에 반영하라는 알림. [게임플레이 구독] */
	UPROPERTY(BlueprintAssignable, Category = "Combat|Input")
	FOnApplyDiceResults OnApplyDiceResults;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief SkillIndex를 그대로 게임플레이에 전달한다. UI는 스킬 객체를 직접 들고 있지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestSelectSkill(int32 SkillIndex);
	/** @brief DiceIndex 선택/해제를 의도로 보낸다. 사용 가능 여부와 값 검증은 게임플레이/어댑터가 수행한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestToggleDice(int32 DiceIndex);
	/** @brief 현재 보유 주사위 굴림 요청만 보낸다. RNG와 결과 push는 구독자가 책임진다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestRollDice();

	/** @brief 입장 물리 굴림 결과면(0-base)을 게임플레이에 반영하라고 알린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestApplyDiceResults(const TArray<int32>& RolledFaceIndices);
	/** @brief SkillIndex 상세 표시 요청을 보낸다. 상세 데이터는 SetSkillDetail()로 되돌아온다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressSkill(int32 SkillIndex);
	/** @brief MOVE 모드 진입 의도만 보낸다. 실제 타일 판정은 월드 터치 입력 뒤 처리된다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestMove();
	/** @brief 턴 종료 버튼 의도. 실제 턴 시스템 호출과 실패 처리는 게임플레이가 맡는다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestEndTurn();
	/** @brief 현재 스킬/주사위/타겟 선택 취소 의도. UI 강조 해제는 OnActionResolved로 되돌아온다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestCancel();
	/** @brief 현재 스킬/이동 빌드의 프리뷰를 확정한다. 대상/목적지가 없으면 게임플레이가 거부한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestConfirm();
	/** @brief 장비 슬롯 상세 요청. SlotIndex는 FEquipmentUI.mSlotIndex와 같은 계약이다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Input") void RequestLongPressEquip(int32 SlotIndex);
	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ─────────
	   각 Set*()은 UI가 그리려면 게임플레이가 반드시 공급해야 하는 값이다(UI는 못 만듦).
	   [소스]=가져올 곳(정해짐), [합의필요]=진짜 소스 미정(현재 Mock/placeholder). */
public:
	/** @brief 전체 유닛 HP/이동력/타일/HP바위치/상태태그. [합의필요] HP/MaxHP 진짜소스=UUnitData(GAS 폐기후), 현재 placeholder. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitUIs(const TArray<FUnitUI>& Units);
	/** @brief 유닛 롱프레스 상세(이름/레벨/초상화/패시브). [합의필요] UUnitData 연결. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetUnitDetail(const FUnitDetailUI& Detail);
	/** @brief 보유 주사위(굴림값/면수/희귀도색/사용잠금/3D프리뷰 슬롯). [소스] APlayerUnit UDicePoolModel(진짜, 비GAS). */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetDiceUIs(const TArray<FDiceSlotUI>& Dice);
	/** @brief 스킬에 올린 주사위 index들+합계. [소스] SRPGSkillBuildAction.mSelectedDices. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSelectedDice(const TArray<int32>& SelectedIndices, int32 SelectedSum);
	/** @brief 보유/선택 주사위 상태를 한 알림으로 교체해 3D 카드 전체가 중복 갱신되지 않게 한다. */
	void SetDiceState(const TArray<FDiceSlotUI>& Dice, const TArray<int32>& SelectedIndices, int32 SelectedSum);
	/** @brief 스킬 레일(이름/아이콘/주사위코스트/사용가능). [합의필요] 소스=USkillComponent(김준형), 현재 Mock. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillUIs(const TArray<FSkillUI>& Skills);

	/** @brief 선택한 스킬 index. [소스] SRPGSkillBuildAction.mSelectedSkill. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSelectedSkill(int32 SelectedIndex);
	/** @brief 스킬 롱프레스 상세. [합의필요] 스킬 데이터 연결. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetSkillDetail(const FSkillDetailUI& Detail);
	/** @brief 턴유닛/라운드/페이즈/턴순서. mPhase=ECombatBuildPhaseUI(UI 전용). [합의필요] AimSelection/Preview만 ESRPGSkillBuildPhase와 매핑(모호재), SkillSelected/DiceSelect는 어댑터 파생. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetTurnUI(const FTurnUI& Turn);
	/** @brief 조작 빌드 페이즈만 갱신(스킬/이동 빌드 공용). 턴 스냅샷 전체 교체 없이 페이즈 전환을 알릴 때 사용. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetBuildPhase(ECombatBuildPhaseUI Phase);
	/** @brief 이동 빌드의 현재 이동 가능 거리만 갱신한다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void SetMoveRange(int32 MoveRange);
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

	/** @brief TurnIndex번째 턴, ActionIndex번째 액션의 MotionEventLogs 배열에서 MotionIndex번째 모션 연출이 끝났음을 알린다. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLogMotionFinished(int32 TurnIndex, int32 ActionIndex, int32 MotionIndex);

	/** @brief 현재/대기 중인 플로팅 로그를 전부 지운다. 시뮬레이션이 다른 것으로 넘어가 미리보기 목록을 통째로 버릴 때 호출. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyCombatFloatingLogsCleared();

	/** @brief 턴 시작 주사위 굴림 오버레이 열기를 요청한다(OnDiceRollRequested). 게임플레이가 DicePrepare 시점에 호출. */
	UFUNCTION(BlueprintCallable, Category = "Combat|Push") void NotifyDiceRollRequested();

	/* ───────── 위젯이 읽는다 ───────── */
public:
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FUnitUI>& GetUnitUIs() const { return mUnitUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FUnitDetailUI& GetUnitDetail() const { return mUnitDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FDiceSlotUI>& GetDiceUIs() const { return mDiceUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<int32>& GetSelectedDiceIndices() const { return mSelectedDiceIndices; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 GetSelectedDiceSum() const { return mSelectedDiceSum; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FSkillUI>& GetSkillUIs() const { return mSkillUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") int32 GetSelectedSkillIndex() const { return mSelectedSkillIndex; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FSkillDetailUI& GetSkillDetail() const { return mSkillDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FCombatQueueNode>& GetActionQueue() const { return mActionQueue; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FTurnUI& GetTurnUI() const { return mTurnUI; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const TArray<FEquipmentUI>& GetEquipmentUIs() const { return mEquipmentUIs; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FEquipmentDetailUI& GetEquipmentDetail() const { return mEquipmentDetail; }
	UFUNCTION(BlueprintPure, Category = "Combat|Read") const FPlayerMetaUI& GetPlayerMeta() const { return mPlayerMeta; }

private:
	/** @brief 마지막으로 push된 유닛 표시 스냅샷. 위젯은 참조로 읽고 수정하지 않는다. */
	UPROPERTY(Transient) TArray<FUnitUI> mUnitUIs;
	/** @brief 마지막 유닛 상세 스냅샷. 롱프레스 상세 패널의 단일 소스다. */
	UPROPERTY(Transient) FUnitDetailUI mUnitDetail;
	/** @brief 마지막 주사위 표시 스냅샷. 굴림값/사용잠금/면 정보가 모두 여기에 모인다. */
	UPROPERTY(Transient) TArray<FDiceSlotUI> mDiceUIs;
	/** @brief 현재 스킬 빌드에 올린 주사위 index 목록. index는 mDiceUIs 배열 기준이다. */
	UPROPERTY(Transient) TArray<int32> mSelectedDiceIndices;
	/** @brief 선택된 주사위 결과 합계. UI가 다시 합산하지 않게 게임플레이가 확정해 push한다. */
	UPROPERTY(Transient) int32 mSelectedDiceSum = 0;
	/** @brief 스킬 레일 표시 스냅샷. SkillIndex payload와 같은 index 계약을 가진다. */
	UPROPERTY(Transient) TArray<FSkillUI> mSkillUIs;
	/** @brief 현재 선택한 스킬 index. index는 mSkillUIs 배열 기준이다. */
	UPROPERTY(Transient) int32 mSelectedSkillIndex = INDEX_NONE;
	/** @brief 마지막 스킬 상세 스냅샷. */
	UPROPERTY(Transient) FSkillDetailUI mSkillDetail;
	/** @brief 아직 재생되지 않은 행동 결과 큐. ResolveFrontQueueNode()가 앞에서 하나씩 제거한다. */
	UPROPERTY(Transient) TArray<FCombatQueueNode> mActionQueue;
	/** @brief 현재 턴/라운드/페이즈 표시 스냅샷. */
	UPROPERTY(Transient) FTurnUI mTurnUI;
	/** @brief 장비 슬롯 표시 스냅샷. */
	UPROPERTY(Transient) TArray<FEquipmentUI> mEquipmentUIs;
	UPROPERTY(Transient) FEquipmentDetailUI mEquipmentDetail;
	/** @brief 플레이어 메타 표시 스냅샷. */
	UPROPERTY(Transient) FPlayerMetaUI mPlayerMeta;
};
