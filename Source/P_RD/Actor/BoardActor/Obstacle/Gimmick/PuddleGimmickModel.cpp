/*****************************************************************//**
 * @file   PuddleGimmickModel.cpp
 * @brief  장판 기믹 모델 구현 파일
 * @author 이문환
 * @date   2026-08-21
 *********************************************************************/

#include "Actor/BoardActor/Obstacle/Gimmick/PuddleGimmickModel.h"

#include "GameplayTagType.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/ObstacleSpawnData/StaticPuddleGimmickSpawnData.h"

UPuddleGimmickModel::UPuddleGimmickModel()
{
	// 새 장판이 같은 타일에 오면 이 장판을 교체(제거)할 수 있게 허용
	// 우선순위는 기본값(0) 그대로 = 나중에 온 장판이 항상 이김
	mReplaceLayerFlags = StaticCast<int32>(ETileLayerFlag::Overlay);
}

void UPuddleGimmickModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// 스폰 데이터에서 라운드 수명 읽기
	const UStaticPuddleGimmickSpawnData* PuddleSpawn = Cast<UStaticPuddleGimmickSpawnData>(mStaticSpawnData);
	if (PuddleSpawn != nullptr)
	{
		mRemainingRoundCount = PuddleSpawn->mRoundLifetime;
	}

	// 장판 일괄 발동 이벤트 등록 시도
	// 테스트 등 전투 모델이 없는 환경에서는 등록 생략
	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	if (CombatModel != nullptr)
	{
		CombatModel->AddUniqueRoundEndEvent(TInstancedStruct<FSRPGCombatRoundEvent>::Make<FPuddleRoundEndEvent>());
	}
}

void UPuddleGimmickModel::OnReplaced(FTile* CurTile, UBoardActorModel* Other)
{
	Super::OnReplaced(CurTile, Other);

	// 타일에서는 이미 빠진 상태. 사망 태그를 붙여 다음 정리 시점에 수거되게 함
	GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
}

void UPuddleGimmickModel::TriggerRoundEnd(TSharedPtr<FPresentationBarrier> PresentationBarrier)
{
	// 이번 라운드 중 교체되었거나 이미 수명이 끝난 장판은 처리하지 않음
	if (IsDead() == true)
	{
		return;
	}

	// 위에 발동 대상(유닛)이 서 있으면 자기 타일을 조준해 스킬 시전
	const FTileIndex TileIndex = GetTileTransform().mIndex;
	const bool IsOccupied = GetTileMap()->GetActorsOnTile(TileIndex, StaticCast<ETileLayerFlag>(mTriggerLayerFlags)).Num() > 0;
	if (IsOccupied == true)
	{
		TryTriggerGimmick(TileIndex, PresentationBarrier);
	}

	// 라운드 수명 차감. 무제한은 음수 유지
	// 스킬 시전 후에 차감하므로 마지막 라운드에도 데미지가 나감
	if (mRemainingRoundCount > 0)
	{
		--mRemainingRoundCount;
	}

	// 수명 소진 시 사망 태그를 붙여 다음 정리 시점에 수거되게 함
	// 방금 시전한 스킬 연출은 배리어가 잡고 있어서 정리 전에 끝까지 재생됨
	if (mRemainingRoundCount == 0)
	{
		GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
	}
}

FPuddleRoundEndEvent::FPuddleRoundEndEvent()
{
	// 한 번 발동하고 끝나는 이벤트가 아니라 라운드 끝마다 반복 발동
	mIsLoop = true;
}

ESRPGCombatRoundEventResult FPuddleRoundEndEvent::Trigger_Internal(TSharedPtr<FPresentationBarrier> RoundBarrier, USRPGCombatModel* Model)
{
	UTileMapModel* TileMap = Model->GetTileMap();
	checkf(TileMap != nullptr, TEXT("타일 맵 nullptr"));

	// 보드 전체에서 장판 수집
	// 발동이 보드 상태를 바꿀 수 있으므로, 순회 중에 발동하지 않고 먼저 모아둠
	TArray<UPuddleGimmickModel*> Puddles;
	for (int32 Y = 0; Y < TileMap->GetHeight(); ++Y)
	{
		for (int32 X = 0; X < TileMap->GetWidth(); ++X)
		{
			Puddles.Append(TileMap->GetActorsOnTile<UPuddleGimmickModel>(FTileIndex(X, Y), ETileLayerFlag::Overlay));
		}
	}

	// 모든 장판을 한꺼번에 발동. 같은 배리어를 공유하므로 동시에 연출되고, 마지막 스킬이 끝나야 라운드가 전환됨
	for (UPuddleGimmickModel* Puddle : Puddles)
	{
		Puddle->TriggerRoundEnd(RoundBarrier);
	}

	// 이번 라운드 처리 종료. 반복 이벤트이므로 다음 라운드 끝에 다시 발동됨
	return ESRPGCombatRoundEventResult::End;
}
