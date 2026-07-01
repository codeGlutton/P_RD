/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "UI/Combat/CombatUITypes.h"
#include "CombatGameMode.generated.h"

struct FEquippedEntry;
struct FPresentationBarrier;
class UCombatUIModel;
class UUnitModel;

class USRPGSkillBuildAction;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

/*
 * 전투 UI 경계 메모 (#216 / #217 리뷰용)
 *
 * #216은 실제로 머지/테스트할 전투 HUD UI PR이다.
 * #217은 이 파일처럼 "UI가 필요해서 작성했지만 GameMode/게임플레이 소유에 가까운 코드"를
 * PM이 검토하거나 가져갈 수 있게 떼어 둔 참고용 PR이다.
 *
 * 이 파일의 public UI 함수와 OnRefresh* 대리자는 위젯이 직접 만지는 경계면이다.
 * 반면 함수 본문에서 커맨드를 만들고, 전투 모델 이벤트를 받아 Push*UI를 호출하는 오케스트레이션은
 * UI 위젯 책임이 아니라 GameMode/게임플레이 책임에 가깝다.
 */
DECLARE_MULTICAST_DELEGATE(FOnRefreshAllUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshUnitUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSkillUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshEquipmentUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshTurnUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshPlayerMetaUI);
DECLARE_MULTICAST_DELEGATE(FOnCombatActionResolvedUI);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCombatPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnCombatEndedUI, TSharedPtr<FPresentationBarrier> /*Barrier*/, bool /*playerWin*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnTurnPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActionPresentationUI, TSharedPtr<FPresentationBarrier> /*Barrier*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshCombatBuildPhase, ECombatBuildPhaseUI /*Phase*/);
DECLARE_MULTICAST_DELEGATE(FOnCombatShowDicePanelUI);
DECLARE_MULTICAST_DELEGATE(FOnCombatShowTargetDetailPanelUI);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowSkillDetailPanelUI, int32 /*SkillIndex*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShowEquipmentDetailPanelUI, int32 /*SlotIndex*/);

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

	/* UI 진입점 */
public:
	/*
	 * UI -> GameMode 입력 경계.
	 *
	 * 위젯은 아래 함수만 호출한다.
	 * 위젯이 FSRPGCommand, CombatModel, DicePoolModel을 직접 만들거나 조회하지 않게 하려는 의도다.
	 * 함수 시그니처 자체는 UI 경계지만, 함수 본문에서 커맨드를 구성/제출하는 부분은
	 * #217에서 PM 검토 대상으로 따로 설명한 GameMode/게임플레이 글루다.
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectSkill(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectDice(int32 DiceIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool RollDices();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectMove();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool EndTurn();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool ShowSkillDetail(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool ShowEquipmentDetail(int32 SlotIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	void PushAllCombatUI();

	bool GetEquipmentUIs(TArray<FEquipmentUI>& OutEquipmentUIs) const;

public:
	/**
	 * @brief 터치 입력 아래의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldTouchEvent();

	/**
	 * @brief 긴 터치 입력 아래의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldLongPressEvent();

public:
	/**
	 * @brief 장비 디테일 정보를 가져온다.
	 * @param EquipmentType 원하는 장비 대상
	 * @return 장비 런타임 정보
	 */
	const FEquippedEntry* GetEquipmentDetail(EEquipmentType EquipmentType);

	/* 연출용 대리자 (전투 수명/턴 연출 배리어 — UI가 구독) */
public:
	/*
	 * 연출 대리자.
	 *
	 * 게임플레이 내부 타입을 UI에 최대한 노출하지 않기 위해 payload를 줄였다.
	 * 예: 전투 종료는 ESRPGCombatResult 대신 bool playerWin만 내려준다.
	 */
	FOnCombatPresentationUI OnBeginCombatUI;
	FOnCombatEndedUI OnEndCombatUI;
	FOnTurnPresentationUI OnBeginAnyTurnUI;
	FOnTurnPresentationUI OnEndAnyTurnUI;
	FOnActionPresentationUI OnBeginAnyTurnActionUI;
	FOnActionPresentationUI OnEndAnyTurnActionUI;

public:
	/*
	 * 표시 갱신 대리자.
	 *
	 * 규칙:
	 * 1. Push*UI()가 UCombatUIModel에 DTO를 저장한다.
	 * 2. 해당 OnRefresh*UI를 Broadcast한다.
	 * 3. HUD/TopBar는 신호를 받으면 UCombatUIModel을 다시 읽어 자기 영역만 다시 그린다.
	 *
	 * 즉 이 대리자들은 새 데이터를 직접 실어 나르는 채널이 아니라
	 * "어느 영역을 다시 읽고 그릴지" 알려주는 신호선이다.
	 */
	FOnRefreshAllUI OnRefreshAllUI;
	FOnRefreshUnitUI OnRefreshUnitUI;
	FOnRefreshDiceUI OnRefreshDiceUI;
	FOnRefreshSelectedDiceUI OnRefreshSelectedDiceUI;
	FOnRefreshSkillUI OnRefreshSkillUI;
	FOnRefreshEquipmentUI OnRefreshEquipmentUI;
	FOnRefreshTurnUI OnRefreshTurnUI;
	FOnRefreshPlayerMetaUI OnRefreshPlayerMetaUI;
	FOnCombatActionResolvedUI OnCombatActionResolvedUI;
	FOnRefreshCombatBuildPhase OnRefreshSkillBuildPhase;
	FOnRefreshCombatBuildPhase OnRefreshMoveBuildPhase;
	FOnCombatShowDicePanelUI OnShowDicePanelAnyTurnUI;
	FOnCombatShowTargetDetailPanelUI OnShowTargetDetailPanelUI;
	FOnShowSkillDetailPanelUI OnShowSkillDetailPanelUI;
	FOnShowEquipmentDetailPanelUI OnShowEquipmentDetailPanelUI;

protected:
	/*
	 * 게임플레이 이벤트 -> UI DTO push 경계.
	 *
	 * 아래 함수들은 위젯 코드가 아니다.
	 * 전투 모델/플레이어 모델/컴포넌트를 읽어 표시 DTO를 만들고 UCombatUIModel에 저장한 뒤,
	 * 도메인별 OnRefresh*UI를 쏘는 GameMode 쪽 배선이다.
	 *
	 * 그래서 #216 기능에는 필요하지만, 최종 소유/리뷰는 #217에서 PM이 확인하기 쉽게 분리했다.
	 */
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);
	void BindPlayerDicePoolUIEvents();
	void BindPlayerMetaUIEvents();

	void PushUnitUI();
	void PushDiceUI();
	void PushSelectedDiceUI();
	void PushTurnUI();
	void PushSkillUI();
	void PushEquipmentUI();
	void PushPlayerMetaUI();

	UCombatUIModel* GetCombatUIModel() const;
};
