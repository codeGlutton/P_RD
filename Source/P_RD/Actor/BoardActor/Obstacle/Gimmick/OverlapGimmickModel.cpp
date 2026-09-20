/*****************************************************************//**
 * @file   OverlapGimmickModel.cpp
 * @brief  진입 트리거 기믹 모델 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#include "Actor/BoardActor/Obstacle/Gimmick/OverlapGimmickModel.h"

UOverlapGimmickModel::UOverlapGimmickModel()
{
	// 유닛이 밟을 수 있어야 하므로 유닛 아래 깔리는 Overlay 레이어 + 통행 비차단으로 재설정
	mTileLayerFlags = StaticCast<int32>(ETileLayerFlag::Overlay);
	mBlockLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::None);
}

bool UOverlapGimmickModel::IsTargetable() const
{
	// 발밑 기믹은 살아 있어도 조준/피격 대상이 아님
	return false;
}

void UOverlapGimmickModel::OnBeginTileOverlap(FTile* CurTile, UBoardActorModel* Other)
{
	Super::OnBeginTileOverlap(CurTile, Other);

	// 발동 대상 레이어가 아니면 무시 (다른 Overlay 액터가 겹쳐 깔리는 경우 등)
	if ((StaticCast<int32>(Other->GetTileLayerFlags()) & mTriggerLayerFlags) == 0)
	{
		return;
	}

	// 밟힌 자리(자기 타일)를 조준해 발동
	TryTriggerGimmick(GetTileTransform().mIndex);
}
