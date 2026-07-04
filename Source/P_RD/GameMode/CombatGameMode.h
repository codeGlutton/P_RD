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
class UCombatUIAdapter;
class UCombatUIModel;
enum class ECombatInputType : uint8;

struct FTacticalAttributeChangeData;

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
	 * @brief UIModel의 조작 의도(OnCombatCommand)를 위 진입점으로 라우팅한다.
	 * @details 위젯은 게임모드를 직접 모른다 — Request*로 의도만 쏘고, 여기서 커맨드 발행 진입점에 연결한다.
	 */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief UIModel의 물리 굴림 결과 주입 의도(OnApplyDiceResults)를 굴림 커맨드로 연결한다. */
	UFUNCTION()
	void HandleApplyDiceResults(const TArray<int32>& RolledFaceIndices);

	/**
	 * @brief 전장 타일을 탭했을 때(UI의 OnCombatWorldTouch) 게임플레이로 넘겨 조준/시전을 진행시킨다.
	 *
	 * @details
	 * 스킬을 골라도 이 선이 없으면 타일 탭이 게임에 도달하지 못해 조준 자체가 안 됐다.
	 * 단, 플레이어의 활성 턴이 아니면 넘기지 않는다 — 넘기면 시전 프리뷰 시뮬레이션이
	 * "현재 전투가 진행 중이 아님" 어설션에 걸린다(구현부 주석 참고).
	 */
	UFUNCTION()
	void HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress);

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
	 * @brief 스킬을 길게 눌렀을 때 그 스킬의 상세(이름/설명/아이콘/코스트/사거리)를 UI로 밀어넣는다.
	 *
	 * @details
	 * 이 생산자가 없어서 상세 카드가 "연결 대기중" 폴백만 띄웠다. UI는 요청 직후 GetSkillDetail()을
	 * 동기로 읽는 계약이라, 요청->이 함수까지 전 구간이 동기 브로드캐스트여서 읽기보다 항상 먼저 실행된다.
	 */
	void PushSkillDetailUIData(int32 SkillIndex) const;
	void PushEquipmentUIData() const;
	void PushPlayerMetaUIData() const;

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

public:
	/** @brief 전투 상태를 CombatUIModel로 push + HUD 입력 의도를 처리하는 임시 비GAS 어댑터(전투 수명 동안 보유). */
    // TODO : 추후 삭제
	UPROPERTY()
	TObjectPtr<UCombatUIAdapter> mCombatUIAdapter;
};
