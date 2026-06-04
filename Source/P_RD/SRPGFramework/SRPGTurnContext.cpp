#include "SRPGFramework/SRPGTurnContext.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"
#include "Singleton/WorldSubsystem/PresentationSyncSubsystem.h"

#include "Pawn/Unit.h"

#include "FunctionLibrary/GASTargetFunctionLibrary.h"

TSharedPtr<FSRPGAction> FSRPGTurnContext::BuildAction(TSharedPtr<FSRPGActionBuilder> Builder)
{
	checkf(Builder->mIsExpired == true, TEXT("만기된 빌더. 재사용 불가"));

	checkf(mPhase == ESRPGTurnPhase::ActionBuild, TEXT("액션 빌드 절차 오류"));
	mPhase = ESRPGTurnPhase::ActionSelect;

	TSharedPtr<FSRPGAction> NewAction = Builder->BuildAction();
	Builder->mIsExpired = true;

	return NewAction;
}

void FSRPGTurnContext::UnbuildAction(TSharedPtr<FSRPGActionBuilder> Builder)
{
	checkf(Builder->mIsExpired == true, TEXT("만기된 빌더. 재사용 불가"));

	checkf(mPhase == ESRPGTurnPhase::ActionBuild, TEXT("액션 빌드 절차 오류"));
	mPhase = ESRPGTurnPhase::ActionSelect;

	Builder->UnbuildAction();
	Builder->mIsExpired = false;
}

void FSRPGTurnContext::PushAction(TSharedPtr<FSRPGAction> NewAction)
{
	mActions.Enqueue(NewAction);

	// 액션 대기 중 경우, 새로운 액션 즉시 진행
	if (mPhase == ESRPGTurnPhase::ActionSelect)
	{
		StartNextAction();
	}
}

void FSRPGTurnContext::InitTurn(AUnit* Owner, int32 LifeCount)
{
	checkf(mPhase == ESRPGTurnPhase::None, TEXT("중복 초기화"));
	mPhase = ESRPGTurnPhase::TurnInit;

	mOwner = Owner;
	mLifeCount = LifeCount;
}

void FSRPGTurnContext::BeginTurn()
{
	checkf(mPhase == ESRPGTurnPhase::TurnInit, TEXT("이미 턴 진행 중에 재실행 오류"));
	mPhase = ESRPGTurnPhase::TurnStart;

	UE_LOG(LogSRPGCombat, Log, TEXT("턴 시작"));
	
	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		// 현 스텟을 캡처
		FGameplayEventData EventData;
		EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner);
		EventData.Instigator = mOwner;
		EventData.Target = mOwner;

		// On Start Turn 패시브 실행
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner, AbilityTags::GameplayAbility_Passive_OnStartTurn, MoveTemp(EventData));
		// 턴 실행
		checkf(mPhase == ESRPGTurnPhase::TurnStart, TEXT("턴 진입 절차 오류"));
		mPhase = ESRPGTurnPhase::ActionSelect;
		
		// 플레이어의 경우 주사위 굴리기 액션 추가
		if (mOwner->IsPlayerUnit() == true)
		{
			// 액션 추가
		}
		// AI의 경우 움직임 판단 로직 시작
		else
		{
			// AI의 경우 움직임 판단 로직 시작
		}
		}));
	OnBeginTurnUI.Broadcast(PresentationBarrier, *this);
}

void FSRPGTurnContext::TickTurn(float DeltaTime)
{
	if (mPhase == ESRPGTurnPhase::ActionPlay && mActions.IsEmpty() == false)
	{
		(*mActions.Peek())->TickAction(DeltaTime);
	}
}

void FSRPGTurnContext::EndTurn()
{
	checkf(mPhase == ESRPGTurnPhase::TurnAbort, TEXT("턴 종료 절차 오류"));
	mPhase = ESRPGTurnPhase::TurnEnd;

	UPresentationSyncSubsystem* PresentationSyncSubsystem = GetWorld()->GetSubsystem<UPresentationSyncSubsystem>();
	checkf(PresentationSyncSubsystem != nullptr, TEXT("연출 동기화 서브시스템 nullptr"));

	// 현 스텟을 캡처
	FGameplayEventData EventData;
	EventData.TargetData = UGASTargetFunctionLibrary::MakeSnapshotTargetDataHandle(mOwner);
	EventData.Instigator = mOwner;
	EventData.Target = mOwner;

	// On End Turn 패시브 실행
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(mOwner, AbilityTags::GameplayAbility_Passive_OnEndTurn, MoveTemp(EventData));

	auto PresentationBarrier = PresentationSyncSubsystem->MakePresentationBarrier(FOnFinishPresentation::CreateSPLambda(AsShared(), [this]() {
		if (IsPermanent() == false)
		{
			--mLifeCount;
		}
		USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
		CombatSubsystem->OnEndCurrentTurn(AsShared(), mResult);

		checkf(mPhase == ESRPGTurnPhase::TurnEnd, TEXT("턴 종료 절차 오류"));
		mPhase = ESRPGTurnPhase::TurnInit;
		mActions.Empty();

		UE_LOG(LogSRPGCombat, Log, TEXT("턴 종료"));
		}));
	OnEndTurnUI.Broadcast(PresentationBarrier, *this, mResult);
}

void FSRPGTurnContext::StartNextAction()
{
	checkf(mPhase == ESRPGTurnPhase::ActionSelect, TEXT("선택 가능 상태에서만 다음 액션으로 통과 가능"));

	if (mActions.IsEmpty() == true)
	{
		return;
	}

	mPhase = ESRPGTurnPhase::ActionPlay;
	(*mActions.Peek())->BeginAction();
}

void FSRPGTurnContext::EvaluateTurnStates(bool ForceAbort)
{
	EvaluateTurnEndState(ForceAbort);

	if (mActions.IsEmpty() == false)
	{
		const bool ForceAbortAction = ForceAbort && mPhase == ESRPGTurnPhase::TurnAbort;
		(*mActions.Peek())->EvaluateActionEndState(ForceAbortAction);
	}
}

void FSRPGTurnContext::OnEndCurrentAction(TSharedRef<FSRPGAction> Action, ESRPGActionResult ActionResult)
{
	mActions.Pop();

	// 턴 소모 액션 처리
	bool IsBlockTurnSuccessfully = Action->IsTurnEndingAction() == true && ActionResult == ESRPGActionResult::Succeeded;
	if (IsBlockTurnSuccessfully == true)
	{
		mPhase = ESRPGTurnPhase::TurnAbort;
		mResult = ESRPGTurnResult::Succeeded;
	}

	// 턴 종료 여부 체크
	if (mPhase == ESRPGTurnPhase::TurnAbort)
	{
		EndTurn();
		return;
	}

	checkf(mPhase == ESRPGTurnPhase::ActionPlay, TEXT("액션 종료 절차 오류"));
	mPhase = ESRPGTurnPhase::ActionSelect;

	StartNextAction();
}

void FSRPGTurnContext::EvaluateTurnEndState(bool ForceAbort)
{
	checkf(mOwner != nullptr, TEXT("턴 주인 미존재"));

	/* 이미 중단 */

	if (mPhase == ESRPGTurnPhase::TurnAbort)
	{
		return;
	}

	/* 외부에서 강제 종료되는가? */
	/* 플레이어가 죽어서 턴이 종료되는가? */

	if (ForceAbort == true || mOwner->IsDead() == true)
	{
		mResult = ESRPGTurnResult::Cancelled;
		mPhase = ESRPGTurnPhase::TurnAbort;
		return;
	}
}

UWorld* FSRPGTurnContext::GetWorld() const
{
	return mOwner->GetWorld();
}

AUnit* FSRPGTurnContext::GetOwner() const
{
	return mOwner;
}

bool FSRPGTurnContext::IsPermanent() const
{
	return mLifeCount == PERMENENT_TURN;
}

int32 FSRPGTurnContext::GetLifeCount() const
{
	return mLifeCount;
}

