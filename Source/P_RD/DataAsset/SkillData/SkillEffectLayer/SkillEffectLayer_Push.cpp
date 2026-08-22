/*****************************************************************//**
 * @file   SkillEffectLayer_Push.cpp
 * @brief  하나의 스킬 모션 내에서 적용하는 밀치기 효과 단위 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer_Push.h"

#include "Actor/TileMap/TileMapModel.h"
#include "Actor/BoardActor/BoardActorModel.h"
#include "Actor/BoardActor/BoardCombatTarget.h"
#include "Component/BoardMovementComponent/BoardMovementComponentModel.h"

void FSkillEffectLayer_Push::CommitEffect(const FSkillEffectCommitParams& Params) const
{
	// 시전자의 보드 정보 획득 (미는 방향 = 시전자가 바라보는 방향, 시전자 타일 = 연쇄 방지 키)
	UBoardActorModel* InstigatorModel = Cast<UBoardActorModel>(Params.mInstigator.GetObject());
	checkf(InstigatorModel != nullptr, TEXT("밀치기 시전자가 보드 액터가 아님"));

	const FTileIndex InstigatorTileIndex = InstigatorModel->GetTileTransform().mIndex;
	const ETileActorDirection PushDirection = InstigatorModel->GetTileTransform().mDirection;

	for (const TScriptInterface<IBoardCombatTarget>& Target : Params.mTargets)
	{
		UBoardActorModel* TargetModel = Cast<UBoardActorModel>(Target.GetObject());
		UBoardMovementComponentModel* TargetMoveComp = Target->GetBoardMovementComponentModel();
		if (TargetModel == nullptr || TargetMoveComp == nullptr)
		{
			continue;
		}

		// 타일맵은 대상의 이동 컴포넌트에서 획득 (같은 보드 위이므로 동일한 맵)
		UTileMapModel* TileMap = TargetMoveComp->GetTileMap();
		checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

		// 밀리는 경로 계산 (뒤가 막히면 막히기 직전까지로 짧아짐)
		const TArray<FTileIndex> PushPath = TileMap->GetPushPath(TargetModel->GetTileTransform().mIndex, PushDirection, mPushDistance);

		// 한 칸도 밀리지 못하면 아무것도 안 함
		if (PushPath.Num() < 2)
		{
			continue;
		}

		if (TargetMoveComp->IsMoving() == true)
		{
			// 이동 중인 대상: 등록만 하고, 이동 루프가 현재 스텝을 마무리하며 남은 경로를 밀치기 경로로 교체
			TargetMoveComp->TryRegisterPendingPush(InstigatorTileIndex, PushPath);
		}
		else
		{
			// 정지 상태 대상: 즉시 밀기 시작 (방 시작 시 발판 위 배치 발동 등)
			TargetMoveComp->PushAlongPath(PushPath);
		}
	}
}

#define LOCTEXT_NAMESPACE "SkillEffectLayer_Push"

FText FSkillEffectLayer_Push::MakeDescription() const
{
	return FText::Format(
		LOCTEXT("PushFormat", "시전자가 바라보는 방향으로 {0}칸 밀어냅니다."),
		FText::AsNumber(mPushDistance)
	);
}

#undef LOCTEXT_NAMESPACE
