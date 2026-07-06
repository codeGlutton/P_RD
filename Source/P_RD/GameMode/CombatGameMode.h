/*****************************************************************//**
 * @file   CombatGameMode.h
 * @brief  전투 방에 대한 GameMode 정의 헤더
 * @author 모호재
 * @date   2026-05-19
 *********************************************************************/

#pragma once

#include "GameMode/RoomGameModeBase.h"
#include "DataAsset/EquipmentData/EquipmentType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "SRPGFramework/SRPGCommand.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "CombatGameMode.generated.h"

class USRPGTurnContext;
class UUnitModel;
class UCombatUIModel;
enum class ECombatInputType : uint8;

struct FTacticalAttributeChangeData;
struct FSRPGTurnEventLog;   // [로컬 테스트] 시뮬 이벤트 로그 → 플로팅 로그 변환용

// RD Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

DECLARE_MULTICAST_DELEGATE(FOnRefreshAllUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshUnitUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshDiceUI);

DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedDiceUI);
DECLARE_MULTICAST_DELEGATE(FOnRefreshSelectedSkillUI);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshSkillBuildPhase, ESRPGSkillBuildPhase /*Phase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnRefreshMoveBuildPhase, ESRPGMoveBuildPhase /*Phase*/);

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	ACombatGameMode();

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

public:
	UCombatUIModel* GetCombatUIModel() const;

	/* UI 진입점 */
public:
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

	/** @brief 물리 굴림 연출이 확정한 면 index들을 굴림 커맨드에 실어 발행한다("보이는 면 = 기록 숫자"). */
	bool ApplyRolledDices(const TArray<int32>& RolledFaceIndices);

	/**
	 * @brief UIModel에서 올라온 버튼 입력을 전투 명령으로 보낸다.
	 */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief UIModel의 물리 굴림 결과 주입 의도(OnApplyDiceResults)를 굴림 커맨드로 연결한다. */
	UFUNCTION()
	void HandleApplyDiceResults(const TArray<int32>& RolledFaceIndices);

	/**
	 * @brief 전장 탭을 조준/시전 입력으로 처리한다.
	 *
	 * @details
	 * 플레이어 턴이 아닐 때는 입력을 무시한다.
	 */
	UFUNCTION()
	void HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress);

public:
	/**
	 * @brief 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldTouchEvent(FVector2D ScreenPosition);

	/**
	 * @brief 긴 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	UFUNCTION(Category = UI, BlueprintCallable)
	bool ResolveWorldLongPressEvent(FVector2D ScreenPosition);

protected:
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);

protected:
	void PushTurnUIData() const;
	void PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const;
	void PushMoveBuildUIData(ESRPGMoveBuildPhase Phase) const;
	void PushUnitUIData() const;
	void PushDiceUIData() const;
	void PushSelectedDiceUIData() const;
	void PushSkillUIData() const;

	/**
	 * @brief 길게 누른 스킬의 상세 정보를 UIModel에 넣는다.
	 *
	 * @details
	 * 이름, 설명, 아이콘, 코스트, 범위 정보를 채운다.
	 */
	void PushSkillDetailUIData(int32 SkillIndex) const;
	void PushEquipmentUIData() const;
	void PushPlayerMetaUIData() const;

	/**
	 * @brief [로컬 테스트] 스킬 시뮬 결과(TurnEventLogs)를 플로팅 로그 요청으로 변환해 UI로 전달한다.
	 * @details 모호재님이 붙일 "연결(구독→변환→NotifyCombatFloatingLogs)"의 최소 구현 예시.
	 *          속성 변화(수치)를 대상 유닛 월드위치에 띄운다. 타입/색/미리보기 규칙 확정은 별도.
	 */
	void PushSimulationFloatingLogs(const TArray<FSRPGTurnEventLog>& TurnEventLogs) const;

	/* 연출용 대리자 */
public:
	FOnBeginCombatUI OnBeginCombatUI;
	FOnEndCombatUI OnEndCombatUI;
	FOnBeginAnyTurnUI OnBeginAnyTurnUI;
	FOnEndAnyTurnUI OnEndAnyTurnUI;
	FOnBeginAnyTurnActionUI OnBeginAnyTurnActionUI;
	FOnEndAnyTurnActionUI OnEndAnyTurnActionUI;

public:
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "CombatUIModel"))
	TObjectPtr<UCombatUIModel> mCombatUIModel;
};
