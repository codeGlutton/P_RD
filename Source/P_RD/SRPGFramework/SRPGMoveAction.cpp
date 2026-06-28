#include "SRPGFramework/SRPGMoveAction.h"

#include "Singleton/WorldSubsystem/SRPGCombatModel.h"

#include "Pawn/UnitModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Actor/TileMap/TileMapModel.h"

FSRPGMoveCommand::FSRPGMoveCommand()
{
    mCommandType = ESRPGCommandType::MoveCast;
    mRequestedAction = USRPGMoveAction::StaticClass();
}

USRPGMoveAction::USRPGMoveAction()
{
    mActionType = ESRPGActionType::InPlayAction;
    mConsumesTurn = false;
}

ESRPGCommandResult USRPGMoveAction::HandleCommand(const TInstancedStruct<FSRPGCommand>& Command)
{
    ESRPGCommandResult Result = Super::HandleCommand(Command);
    if (Result == ESRPGCommandResult::Handled)
    {
        return Result;
    }

    /* 생성 시 예약된 이동 명령에서 확정 경로를 수신 (빌드 액션이 실어 보낸 경로) */
    if (Command.Get().GetCommandType() == ESRPGCommandType::MoveCast)
    {
        mPathTileIndexes = Command.Get<FSRPGMoveCommand>().mPathTileIndexes;
        return CombineSRPGCommandResult(ESRPGCommandResult::Handled, Result);
    }

    return ESRPGCommandResult::Ignored;
}

void USRPGMoveAction::OnBeginAction()
{
    Super::OnBeginAction();

    /* 경로가 없거나 시작 타일만 있으면 이동 없이 즉시 종료 */
    if (mPathTileIndexes.Num() < 2)
    {
        MarkActionCompleted(ESRPGActionResult::Succeeded);
        return;
    }

    /* 인덱스 0은 시작(현재) 타일이므로 1번 칸부터 이동 시작 */
    StartStep(1);
}

void USRPGMoveAction::OnTickAction(float DeltaTime)
{
    Super::OnTickAction(DeltaTime);

    if (mCurrentStepIndex == INDEX_NONE)
    {
        return;
    }

    /* 한 칸 이동 연출 시간이 지나면 도착 처리 후 다음 칸으로 진행 */
    mStepElapsed += DeltaTime;
    if (mStepElapsed < STEP_DURATION)
    {
        return;
    }

    CompleteStep();

    const int32 NextStepIndex = mCurrentStepIndex + 1;
    if (mPathTileIndexes.IsValidIndex(NextStepIndex) == true)
    {
        StartStep(NextStepIndex);
    }
    else
    {
        /* 경로 끝까지 도착 — 이동 종료 */
        mCurrentStepIndex = INDEX_NONE;
        MarkActionCompleted(ESRPGActionResult::Succeeded);
    }
}

void USRPGMoveAction::OnEndAction()
{
    Super::OnEndAction();

    /* 이동을 정상 완료한 경우, 사용한 이동 포인트를 이동 유닛에게 차감 통지한다 */

    if (mActionResult == ESRPGActionResult::Succeeded && mPathTileIndexes.Num() >= 2)
    {
        // 소모 이동 포인트 = 밟은 칸 수 (경로 칸 수 - 시작 타일)
        const int32 SpentPoint = mPathTileIndexes.Num() - 1;

        // 이동 유닛의 스킬 컴포넌트에 소모 이동 포인트 차감 통지
        if (USkillComponentModel* SkillComp = mInstigator->GetSkillComponentModel())
        {
            SkillComp->HandelMovePoint(SpentPoint);
        }
    }
}

void USRPGMoveAction::StartStep(int32 StepIndex)
{
    checkf(mPathTileIndexes.IsValidIndex(StepIndex) == true, TEXT("이동 경로 인덱스 오류"));

    UTileMapModel* TileMap = GetTileMap();

    // 다음 타일로 이동 시작 (모델 점유는 즉시, 도착 오버랩 통지는 CompleteStep에서)
    // TODO : 진행 방향에 맞춰 바라보는 방향 설정. 임시로 현재 방향 유지.
    const ETileActorDirection Direction = mInstigator->GetTileTransform().mDirection;
    const FTileTransform NextTransform(mPathTileIndexes[StepIndex], Direction);
    TileMap->StartActorMovement(NextTransform, mInstigator.Get());

    mCurrentStepIndex = StepIndex;
    mStepElapsed = 0.0f;
}

void USRPGMoveAction::CompleteStep()
{
    // 현재(도착) 타일의 오버랩 통지 — 함정/장판 등 타일 효과가 여기서 발동
    GetTileMap()->CompleteActorMovement(mInstigator.Get());
}

UTileMapModel* USRPGMoveAction::GetTileMap() const
{
    // 전투 모델을 월드 서브시스템에서 바로 받아 타일 맵을 꺼낸다 (턴 컨텍스트 체인 의존 제거)
    USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
    checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

    UTileMapModel* TileMap = CombatModel->GetTileMap();
    checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));
    return TileMap;
}
