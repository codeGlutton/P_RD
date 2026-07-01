/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "UI/Combat/CombatUITypes.h"
#include "CombatGameMode.generated.h"

struct FEquippedEntry;
struct FSkillEntry;
struct FPresentationBarrier;
class UCombatUIModel;
class UUnitModel;

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

/*
 * 전투 UI 경계 메모 (#216 / #217 리뷰용)
 *
 * #216은 실제로 머지/테스트할 전투 HUD UI PR이다.
 * #217은 이 파일처럼 "UI가 필요해서 작성했지만 GameMode/게임플레이 소유에 가까운 코드"를
 * PM이 검토하거나 가져갈 수 있게 떼어 둔 참고용 PR이다.
 *
 * 이 파일의 public UI 함수는 위젯이 직접 호출하는 입력 경계면이다.
 * 표시 갱신은 GameMode가 HUD에 직접 Broadcast하지 않고 UCombatUIModel::Set*()으로 저장한다.
 * UIModel이 "어느 도메인이 바뀌었는지" 알리고, HUD는 UIModel을 다시 읽어 자기 화면을 그린다.
 *
 * 즉 현재 책임은 아래처럼 나뉜다.
 * - GameMode/Projection: 게임플레이 데이터를 UI 친화적 DTO로 정제해서 UIModel에 저장
 * - UCombatUIModel: DTO 저장 + 변경 알림
 * - HUD/WBP: UIModel을 읽어 표시
 */

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
	 * @brief 스킬 디테일 정보를 가져온다.
	 * @param SkillIndex 원하는 스킬 슬롯
	 * @return 스킬 런타임 정보
	 */
	const FSkillEntry* GetSkillDetail(int32 SkillIndex);

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
	 * SRPGCombatModel에 이미 정의된 연출 신호 타입을 그대로 쓴다.
	 * UI가 연출 barrier/turn/action/result payload를 받을 수 있게 GameMode는 중계만 한다.
	 */
	FOnBeginCombatUI OnBeginCombatUI;
	FOnEndCombatUI OnEndCombatUI;
	FOnBeginAnyTurnUI OnBeginAnyTurnUI;
	FOnEndAnyTurnUI OnEndAnyTurnUI;
	FOnBeginAnyTurnActionUI OnBeginAnyTurnActionUI;
	FOnEndAnyTurnActionUI OnEndAnyTurnActionUI;

protected:
	/*
	 * 게임플레이 이벤트 -> UI DTO push 경계.
	 *
	 * 아래 함수들은 위젯 코드가 아니다.
	 * 전투 모델/플레이어 모델/컴포넌트를 읽어 표시 DTO를 만들고 UCombatUIModel에 저장한다.
	 * 변경 알림은 UIModel Set*() 내부에서 발행하므로 GameMode는 HUD redraw 대리자를 직접 소유하지 않는다.
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
