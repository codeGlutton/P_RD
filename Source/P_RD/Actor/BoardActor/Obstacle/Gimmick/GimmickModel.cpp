/*****************************************************************//**
 * @file   GimmickModel.cpp
 * @brief  기믹 공통 모델 구현 파일
 * @author 이문환
 * @date   2026-08-20
 *********************************************************************/

#include "Actor/BoardActor/Obstacle/Gimmick/GimmickModel.h"

#include "GameplayTagType.h"
#include "Singleton/WorldSubsystem/SRPGCombatModel.h"
#include "Component/SkillComponent/SkillComponentModel.h"
#include "Component/AttributeComponent/AttributeSetComponentModel.h"
#include "DataAsset/ObstacleSpawnData/StaticGimmickSpawnData.h"

void UGimmickModel::PostInitializeComponentModels()
{
	Super::PostInitializeComponentModels();

	// 스폰 데이터 획득 (기믹 스폰 데이터가 아니면 기본값으로 동작)
	const UStaticGimmickSpawnData* GimmickSpawn = Cast<UStaticGimmickSpawnData>(mStaticSpawnData);
	if (GimmickSpawn == nullptr)
	{
		return;
	}

	// 발동 설정 읽기 (양수 = 횟수, 음수 = 무제한. 0은 에셋 저장 검사가 차단)
	mRemainingTriggerCount = GimmickSpawn->mTriggerCount;
	mTriggerSkillIndex = GimmickSpawn->mTriggerSkillIndex;
}

bool UGimmickModel::TryTriggerGimmick(const FTileIndex& AimedTileIndex)
{
	// 수명을 다 썼거나 죽은 기믹은 발동 불가
	if (mRemainingTriggerCount == 0 || IsDead() == true)
	{
		return false;
	}

	USkillComponentModel* SkillComp = GetSkillComponentModel();
	checkf(SkillComp != nullptr, TEXT("스킬 컴포넌트 nullptr"));

	// 시전 중 재진입 방지 (지속형 기믹이 연출 중에 다시 트리거되는 경우)
	if (SkillComp->IsAnySkillActivated() == true)
	{
		return false;
	}

	USRPGCombatModel* CombatModel = GetWorldSubsystemModel<USRPGCombatModel>(this);
	checkf(CombatModel != nullptr, TEXT("전투 모델 nullptr"));

	// 수명 차감 (무제한은 음수 유지)
	// 뷰가 없으면 시전이 아래 호출 안에서 동기로 끝나므로, 종료 콜백이 소진을 볼 수 있게 먼저 차감
	if (mRemainingTriggerCount > 0)
	{
		--mRemainingTriggerCount;
	}

	// 장착된 스킬 강제 시전 (기믹은 행동력 개념이 없으므로 소모 검사 없이 시전)
	FOnEndSkillUI EndCallback;
	EndCallback.AddUObject(this, &UGimmickModel::OnGimmickSkillEnd);
	SkillComp->ForcedActivateSkill(CombatModel->GetTileMap(), mTriggerSkillIndex, AimedTileIndex, EndCallback);
	return true;
}

void UGimmickModel::OnGimmickSkillEnd(const FActiveSkillContext& Context, const UStaticSkillData* SkillData)
{
	// 수명을 다 썼으면 죽음 상태로 표시 (유닛 사망과 같은 태그, IsDead()가 이 태그를 봄)
	// 실제 제거는 이번 행동이 끝날 때 ClearDeadActorModels가 죽은 액터들을 치우면서 함께 처리됨
	if (mRemainingTriggerCount == 0)
	{
		GetAttributeComponentModel()->AddLooseGameplayTag(EffectTags::GameplayEffect_ActorState_Dead);
	}
}
