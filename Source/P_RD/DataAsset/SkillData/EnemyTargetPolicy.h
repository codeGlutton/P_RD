#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "EnemyTargetPolicy.generated.h"

UENUM(BlueprintType)
enum class EEnemyTargetPriority : uint8
{
	Nearest					UMETA(DisplayName = "Nearest", ToolTip = "경로 거리가 가까운 대상 (기존 동작)"),
	LowestHP				UMETA(DisplayName = "LowestHP", ToolTip = "현재 체력이 가장 낮은 대상"),
	LowestHPPercent			UMETA(DisplayName = "LowestHPPercent", ToolTip = "최대 체력 대비 남은 체력 비율이 가장 낮은 대상"),
	HighestMaxHP			UMETA(DisplayName = "HighestMaxHP", ToolTip = "최대 체력이 가장 높은 대상"),
	Farthest				UMETA(DisplayName = "Farthest", ToolTip = "경로 거리가 먼 대상. 경로가 없는 대상은 후순위"),
	Random					UMETA(DisplayName = "Random", ToolTip = "룸 이벤트 시드를 사용하는 무작위 대상"),
	WithStatus				UMETA(DisplayName = "WithStatus", ToolTip = "StatusTag 상태가 있는 대상을 우선 공격"),
	WithoutStatus			UMETA(DisplayName = "WithoutStatus", ToolTip = "StatusTag 상태가 없는 대상을 우선 공격"),
	MostTargets				UMETA(DisplayName = "MostTargets", ToolTip = "이번 턴 스킬 범위에 들어오는 용병 수 최대화. 공격 불가 시 가까운 대상 기준"),
};

UENUM(BlueprintType)
enum class EEnemyUnreachableTargetBehavior : uint8
{
	AttackAvailable			UMETA(DisplayName = "AttackAvailableTarget", ToolTip = "이번 턴 공격할 수 있는 후보 중 우선순위를 적용"),
	ChasePreferred			UMETA(DisplayName = "ChasePreferredTarget", ToolTip = "전체 후보에서 우선 대상을 고르고, 우선순위 높은 후보를 무조건 추격함"),
};

USTRUCT(BlueprintType)
struct P_RD_API FEnemyTargetPolicy
{
	GENERATED_BODY()

public:
	static const FEnemyTargetPolicy Default;

public:
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Priority"))
	EEnemyTargetPriority mPriority = EEnemyTargetPriority::Nearest;

	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UnreachableBehavior", EditCondition = "mPriority != EEnemyTargetPriority::MostTargets", EditConditionHides))
	EEnemyUnreachableTargetBehavior mUnreachableBehavior = EEnemyUnreachableTargetBehavior::AttackAvailable;

	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "StatusTag", EditCondition = "mPriority == EEnemyTargetPriority::WithStatus || mPriority == EEnemyTargetPriority::WithoutStatus", EditConditionHides))
	FGameplayTag mStatusTag;

public:
	bool IsStatusPolicy() const
	{
		return mPriority == EEnemyTargetPriority::WithStatus || mPriority == EEnemyTargetPriority::WithoutStatus;
	}
};

USTRUCT(BlueprintType)
struct P_RD_API FEnemyTargetPolicyOverride
{
	GENERATED_BODY()

public:
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "IsOverrided"))
	bool mIsOverrided = false;
	UPROPERTY(Category = "AI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetPolicy", EditCondition = "mIsOverrided == true", EditConditionHides))
	FEnemyTargetPolicy mTargetPolicy;
};

