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
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Simulation/Logger/EventLog.h"
#include "UI/Reward/RewardUITypes.h"
#include "CombatGameMode.generated.h"

class USRPGTurnContext;
class UUnitModel;
class IBoardSelectionTarget;

class UCombatUIModel;
class URewardUIModel;
class UPlayerUnitModel;
class USkillComponentModel;
class UStaticSkillData;
struct FSkillDetailUI;
class USRPGMoveBuildAction;
struct FTileIndex;
enum class ECombatInputType : uint8;

// Combat Game Mode 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogCombatGameMode, Log, All)

/**
 * @brief  전투 방에 대한 GameMode
 */
UCLASS(abstract)
class P_RD_API ACombatGameMode : public ARoomGameModeBase
{
	GENERATED_BODY()

public:
	ACombatGameMode();

public:
	void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

protected:
	void InitializeRoom() override;
	void BeginRoom() override;

public:
	UCombatUIModel* GetCombatUIModel() const;
	URewardUIModel* GetRewardUIModel() const;

	/* UI 진입점 */
public:
	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectSkill(int32 SkillIndex);

	UFUNCTION(Category = UI, BlueprintCallable)
	bool SelectMove();

	UFUNCTION(Category = UI, BlueprintCallable)
	bool EndTurn();

	/**
	 * @brief UIModel에서 올라온 버튼 입력을 전투 명령으로 보낸다.
	 */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/**
	 * @brief 전장 탭을 조준/시전 입력으로 처리한다.
	 * @details
	 * 플레이어 턴이 아닐 때는 입력을 무시한다.
	 */
	UFUNCTION()
	void HandleCombatWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	UFUNCTION()
	void HandleRewardClaimed(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	UFUNCTION()
	void HandleAbandonRun();

	/** @brief 전투 HUD 상단 가방 버튼에서 공용 인벤토리 패널을 연다. */
	UFUNCTION()
	void HandleOpenInventory();

protected:
	/**
	 * @brief 보상 지급 요청을 검증하고 실제 런 데이터에 반영한다.
	 * @return 지급이 완료되어 UI가 해당 행을 수령 처리해도 되는 경우 true.
	 */
	bool ClaimCombatReward(ERewardClaimKind ClaimKind, int32 ChoiceIndex);

	/**
	 * @brief 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	bool ResolveWorldTouchEvent(FVector2D ScreenPosition);

	/** @brief 겨냥해 둔 칸을 그대로 다시 누른다. 확정 단추가 부른다. */
	void ConfirmTargetTile();

	/**
	 * @brief 긴 터치/클릭 지점의 월드 액터를 검사하여 이벤트를 실행한다.
	 * @param ScreenPosition 입력 지점의 화면 좌표(픽셀). 모바일 터치는 커서가 없으므로 이 좌표로 트레이스한다.
	 * @return 이벤트 성공 여부
	 */
	bool ResolveWorldLongPressEvent(FVector2D ScreenPosition);

	/**
	 * @brief 길게 눌러 고른 대상이 적이면 그 위협 범위를 판에 칠한다.
	 *
	 * @details
	 * 상세 패널과 같은 입력에서 함께 뜬다 -- 적을 살펴보는 이유가 "다음 턴에
	 * 어디까지 오나" 라서, 패널만 띄우고 판을 안 칠하면 반쪽이다.
	 * 아군이나 유닛이 아닌 것을 고르면 이전 칠을 지우기만 한다.
	 *
	 * 예산은 RechargeMovement(턴 시작 충전량)다. 적 자신의 턴 도중 잔량이 아니라
	 * "다음 턴에 받을 행동력" 기준이어야 플레이어 계획에 맞는다.
	 */
	void ShowThreatRangeForTarget(IBoardSelectionTarget* Target) const;

	/** @brief 위협 범위 칠을 지운다. 행동이 시작되면 칠해 둔 전제가 낡는다. */
	void ClearThreatRangeView() const;

protected:
	void OnRegisterUnit(UUnitModel* Unit);
	void OnUnregisterUnit(UUnitModel* Unit);

protected:
	void PushCombatResultUIData(ESRPGCombatResult Result) const;
	void PushTurnUIData() const;
	void PushSkillBuildUIData(ESRPGSkillBuildPhase Phase) const;
	void PushMoveBuildUIData(const USRPGMoveBuildAction* Action, ESRPGMoveBuildPhase Phase) const;
	void PushUnitUIData() const;
	void PushSkillUIData() const;
	void PushSelectedSkillUIData(int32 SkillIndex) const;

	/**
	 * @brief 길게 눌러 고른 대상의 상세를 UI 에 내린다.
	 *
	 * @details
	 * 내려 준 유닛을 mDetailUnitModel 에 적어 둔다. 상세창의 스킬 칸을 탭하면
	 * UI 는 SkillIndex 만 보내오므로, 어느 유닛의 스킬인지는 여기서 기억한
	 * 것으로 되짚는다 -- 화면이 유닛을 짚으면 기준이 두 곳에 생긴다.
	 */
	void PushCombatTargetDetailUIData(IBoardSelectionTarget* Target);

	/** @brief 상세창에 뜬 유닛의 SkillIndex번째 스킬 상세를 내린다(적 스킬일 수도 있다). */
	void PushUnitSkillDetailUIData(int32 SkillIndex) const;

	/** @brief 지금 상세창에 뜬 유닛. 상세창 스킬 칸 탭을 되짚는 기준이다. */
	TWeakObjectPtr<UUnitModel> mDetailUnitModel;

	/** @brief 톡 쳐서 고른 칸을 UI 에 내린다. 스킬 표시값도 같이 다시 내린다. */
	void PushCombatTargetUIData(const FTileIndex& Tile, AActor* HitActor);

	/** @brief 겨냥을 풀고 화면을 겨냥하기 전으로 되돌린다. */
	void ClearCombatTargetUIData();

	/** @brief 지금 겨냥한 자리에 이 스킬을 쓸 수 있나. 행동력과 사거리를 본다. */
	bool IsSkillUsableOnTarget(const UPlayerUnitModel* PlayerUnitModel,
		const UStaticSkillData& StaticSkillData) const;

	/**
	 * @brief 지금 카드에 스킬을 보여 줄 유닛. 없으면 지금 차례인 유닛.
	 *
	 * 하단 용병 칸을 눌러 남의 스킬을 들여다보는 동안에만 값이 들어 있다.
	 * 차례가 넘어가면 지운다 -- 새 차례가 왔는데 옛 유닛 카드가 떠 있으면
	 * 무엇을 조종하는 중인지 알 수 없다.
	 */
	int32 mInspectedUnitId = INDEX_NONE;

	/** @brief 파티에서 이 id 의 유닛을 찾는다. 없으면 nullptr. */
	UPlayerUnitModel* FindPartyUnitModel(int32 UnitId) const;

	/**
	 * @brief 지금 차례인 아군.
	 *
	 * @details
	 * 0번 유닛을 지금 차례로 치던 자리가 여럿 있었다. 파티가 한 명일 때는
	 * 맞았지만 셋이 되면 틀린다 -- 야만전사 차례에 기사의 카드가 떴다.
	 *
	 * 차례는 전투 모델이 안다. 적 차례면 nullptr 이다.
	 * @return 지금 차례인 아군, 적 차례거나 없으면 nullptr
	 */
	UPlayerUnitModel* GetTurnPlayerUnitModel() const;

	void PushSkillDetailUIData(int32 SkillIndex) const;

	/**
	 * @brief 스킬 하나의 상세 표시값을 채운다.
	 *
	 * 카드 레일(조종 중인 아군)과 상세창(들여다보는 유닛)이 같은 내용을 쓰므로
	 * 채우는 자리를 하나로 둔다. 기준 유닛만 호출부가 정한다.
	 */
	void FillSkillDetailUIData(USkillComponentModel* SkillComponentModel,
		int32 SkillIndex, OUT FSkillDetailUI& OutDetail) const;
	void PushEquipmentUIData() const;
	void PushEquipmentDetailUIData(int32 EquipmentIndex) const;
	void PushPlayerMetaUIData() const;

	void PushSimulationFloatingLogs(const TArray<FSRPGTurnEventLog>& TurnEventLogs, bool IsPreview = true) const;

	void PushCombatRewardUIData() const;
	void PushCombatRewardChoicesUIData() const;

public:
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "CombatUIModel"))
	TObjectPtr<UCombatUIModel> mCombatUIModel;
	UPROPERTY(Category = "UI", VisibleAnywhere, DuplicateTransient, meta = (DisplayName = "RewardUIModel"))
	TObjectPtr<URewardUIModel> mRewardUIModel;

private:
	/** @brief 다음 액션/턴이 시작돼 더 이상 유효하지 않은 카메라 복귀 대기를 취소한다. */
	void CancelPendingActionEndAfterCameraReturn();

	/** @brief 카메라 복귀 뒤 보낼 현재 액션 종료 알림. 한 번에 최신 하나만 유지한다. */
	FDelegateHandle mPendingActionEndAfterCameraReturnHandle;

	/** @brief 같은 결과 화면에서 빠른 중복 입력으로 보상이 두 번 지급되는 것을 막는다. */
	bool mGoldRewardClaimed = false;
	bool mExpRewardClaimed = false;
	TSet<int32> mClaimedRewardChoiceIndices;
};
