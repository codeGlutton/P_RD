// @file CombatUIAdapter.h
// @brief 전투 주사위 표시값·타일 하이라이트를 비GAS 소스에서 뷰모델로 push하고, UI 입력을 전투 액션으로 넘기는 어댑터
// @date 2026-06-16

#pragma once

#include "RDMinimal.h"
#include "SRPGFramework/SRPGFrameworkType.h"   // FTileIndex
#include "Actor/TileMap/TileHighlight.h"       // ETileHighlightFlag

#include "CombatUIAdapter.generated.h"

class UCombatUIModel;
class USRPGCombatSubsystem;
class URunPersistData;
class UDicePoolModel;
class ATileMap;
struct FPresentationBarrier;
class USRPGTurnContext;
enum class ECombatInputType : uint8;
enum class ESRPGTurnResult : uint8;

/** @brief 전투 HUD의 주사위 표시값과 타일 하이라이트만 담당하는 비GAS 어댑터입니다. */
// 유닛/메타/턴/이동/공격 등 "가짜 전투 상태 계층"은 제거됨.
//  - 실제 유닛/타일 데이터: Model(UUnitModel / UTileMapModel 등)이 소스.
//  - 입력(스킬 선택/주사위 배치/이동/타일 확정) 처리: USRPGSkillBuildAction이 담당.
// 현재 어댑터가 남기는 책임은 "굴린 주사위 → FDiceSlotUI push"와 "타일 하이라이트 그리기"뿐이다.
// TODO(액션 연동): HandleCombatCommand / HandleWorldTouch를 SRPGSkillBuildAction 커맨드로 라우팅.
UCLASS(BlueprintType)
class P_RD_API UCombatUIAdapter : public UObject
{
	GENERATED_BODY()

public:
	/** @brief 전투 서브시스템/런 데이터를 연결하고 턴 종료 구독 + 초기 주사위 push를 수행한다. */
	void Build(USRPGCombatSubsystem* InCombat, const URunPersistData* InRun);

	/** @brief 뷰모델에 연결하고(입력 구독) 현재 주사위 표시값을 push한다. */
	void BindUIModel(UCombatUIModel* InUIModel);

	/** @brief 플레이어 다이스 컴포넌트를 연결한다(굴림 구동 + 쓴 주사위 잠금 + 뷰 push). */
	// CombatGameMode/HUD 배선이 채운다. 어댑터는 소유하지 않고 런타임 상태를 읽어 FDiceSlotUI로 변환한다.
	void SetDicePool(UDicePoolModel* InDiceComponent) { mDicePool = InDiceComponent; }

protected:
	virtual void BeginDestroy() override;

private:
	/** @brief UI 의도(스킬 선택/주사위 배치/이동/취소)를 받는다. TODO: SRPGSkillBuildAction 커맨드로 라우팅. */
	UFUNCTION()
	void HandleCombatCommand(ECombatInputType Type, int32 IntPayload);

	/** @brief 화면 터치 좌표를 타일로 판정한다. TODO: SRPGSkillBuildAction WorldTrace 커맨드로 라우팅. */
	UFUNCTION()
	void HandleWorldTouch(FVector2D ScreenPosition, bool bLongPress);

	/** @brief 턴 종료 시 '쓴 주사위' 잠금을 해제한다(다음 턴 재사용 가능). Barrier는 붙잡지 않아 즉시 해제. */
	void HandleEndAnyTurn(TSharedPtr<FPresentationBarrier> Barrier, const USRPGTurnContext* TurnContext, ESRPGTurnResult Result);

	/** @brief 뷰모델의 주사위 뷰에서 해당 index 굴림값을 읽는다(안 굴렸으면 0). */
	// UIModel에 push된 결과를 기준으로 판단해 UI가 보는 값과 액션 값이 갈라지지 않게 한다.
	int32 GetRolledDiceValue(int32 DiceIndex) const;

	/** @brief 다이스 컴포넌트를 굴리고(임시 난수 스트림) 그 결과를 뷰모델에 push한다. */
	void RollDice();

	/** @brief 컴포넌트의 현재 주사위 상태를 FDiceSlotUI[]로 변환해 UIModel->SetDiceUIs()로 push한다. */
	void PushDiceUIs() const;

	/** @brief 진행 중이던 하이라이트를 끄고 UI 선택 강조 해제를 알린다(바깥 탭 취소 등). */
	void ClearPendingAction();

	/** @brief 화면 좌표를 타일맵 평면에 투영해 타일 인덱스를 구한다. 타일맵 밖이면 false. */
	// UI는 스크린 좌표만 보내고, 카메라/타일맵 기준 해석은 어댑터/게임플레이 경계에서 수행한다.
	bool ResolveTileFromScreen(const FVector2D& ScreenPosition, FTileIndex& OutTile) const;

	/** @brief 한 타일만 지정 하이라이트(Aim=회색/Select=노랑/Effect=빨강)로 켠다(나머지는 끔). */
	void SetSingleTileHighlight(const FTileIndex& Tile, ETileHighlightFlag Flag) const;

	/** @brief 모든 하이라이트를 끈다. */
	void ClearAllHighlight() const;

private:
	/** @brief UI 위젯들이 공유하는 표시/입력 계약 객체; 어댑터는 여기에 push하고 request를 구독한다. */
	UPROPERTY(Transient)
	TObjectPtr<UCombatUIModel> mUIModel;

	/** @brief 전투 월드 상태 접근점; 타일맵/턴 종료 이벤트만 읽는다. */
	UPROPERTY(Transient)
	TObjectPtr<USRPGCombatSubsystem> mCombat;

	/** @brief 플레이어가 가진 실제 런타임 주사위 묶음. 굴림/사용 잠금의 진짜 소스다. */
	UPROPERTY(Transient)
	TObjectPtr<UDicePoolModel> mDicePool;

	/** @brief OnEndAnyTurnUI(턴 종료) 구독 핸들. 해제/재빌드 때 Remove에 사용. */
	FDelegateHandle mEndTurnHandle;
};
