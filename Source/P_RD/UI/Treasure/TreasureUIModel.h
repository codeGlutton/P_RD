/*****************************************************************//**
 * @file   TreasureUIModel.h
 * @brief  보물방 화면 UI와 게임플레이를 잇는 경계(뷰모델) 정의 헤더
 * @author 이문환
 * @date   2026-08-04
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "UI/Treasure/TreasureUITypes.h"
#include "TreasureUIModel.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTreasureUIChanged, ETreasureUIDomain, Domain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreasureOpenRequested);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTreasureLeaveRequested);

/** @brief 보물방 화면 뷰모델. 보물방 진입 시 게임플레이가 하나 만들어 위젯에 바인딩 */
// 게임플레이가 SetTreasure()로 밀어넣고 위젯은 GetTreasure()로 읽는 구조
// 개봉/나가기는 의도만 전달 — 보상 지급은 게임플레이가 처리 후 SetTreasure로 갱신
UCLASS(BlueprintType)
class P_RD_API UTreasureUIModel : public UObject
{
	GENERATED_BODY()

	/* ───────── 위젯이 구독하는 알림 ───────── */
public:
	/**
	 * @brief 보물방 표시값이 설정/갱신됐음을 알림 (어느 도메인이 갱신됐는지 함께 전달)
	 * @details 위젯은 자기 도메인 알림만 받아 해당 부분을 다시 그림
	 */
	UPROPERTY(BlueprintAssignable, Category = "Treasure|UI")
	FOnTreasureUIChanged OnUIChanged;

	/* ───────── 게임플레이가 구독하는 입력(의도) ───────── */
public:
	/** @brief 위젯이 상자 개봉을 확정했음. 보상 지급은 게임플레이 담당 */
	UPROPERTY(BlueprintAssignable, Category = "Treasure|Input")
	FOnTreasureOpenRequested OnOpenRequested;

	/** @brief 위젯이 보물방을 나가려 함(다음 화면으로). */
	UPROPERTY(BlueprintAssignable, Category = "Treasure|Input")
	FOnTreasureLeaveRequested OnLeaveRequested;

	/* ───────── UI → gameplay : 의도만 보낸다 ───────── */
public:
	/** @brief 상자 개봉 의도를 게임플레이 구독자에게 전달 (개봉 연출은 화면이 먼저 처리) */
	UFUNCTION(BlueprintCallable, Category = "Treasure|Input") void RequestOpen();

	/** @brief 보물방 나가기 의도를 전달 */
	UFUNCTION(BlueprintCallable, Category = "Treasure|Input") void RequestLeave();

	/* ───────── gameplay → UI : 표시값을 밀어넣는다 ───────── */
public:
	/** @brief 게임플레이가 확정한 보물방 표시 스냅샷을 저장하고 변경 알림 발신 */
	UFUNCTION(BlueprintCallable, Category = "Treasure|Push") void SetTreasure(const FTreasureUI& Treasure);

	/* ───────── 위젯이 읽는다 ───────── */
public:
	/** @brief 현재 보물방 스냅샷 반환; 참조는 다음 SetTreasure()까지 유효 */
	UFUNCTION(BlueprintPure, Category = "Treasure|Read") const FTreasureUI& GetTreasure() const { return mTreasure; }

private:
	/** @brief 마지막으로 Push된 보물방 표시값; Transient라 세이브/에셋 상태로 남기지 않음 */
	UPROPERTY(Transient) FTreasureUI mTreasure;
};
