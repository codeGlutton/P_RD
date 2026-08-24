/*****************************************************************//**
 * @file   StaticSkillData.h
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset 구현 헤더
 * @author 모호재
 * @date   2026-05-18
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "DataAsset/PrimaryAssetType.h"
#include "DataAsset/UnitSpawnData/UnitJobType.h"
#include "DataAsset/SkillData/SkillType.h"
#include "SRPGFramework/SRPGFrameworkType.h"
#include "Actor/TileMap/TileLayer.h"
#include "DataAsset/SkillData/SkillEffectLayer/SkillEffectLayer.h"
#include "StaticSkillData.generated.h"

class IBoardCombatTarget;
class UTileMapModel;
class UTacticalEffect_Cooldown;

/**
* @brief 스킬 실행 시 재생될 애니메이션 세트
*/
USTRUCT(BlueprintType)
struct FSkillAnimationSet
{
    GENERATED_BODY()

public:
    /**
    * @brief 타격 처리 시에 활용할 애니메이션들 구분 태그. 연속으로 실행
    * @details
    * (ex: GameplayAnim.Attack, GameplayAnim.Spell)
    */
    UPROPERTY(Category = "Motion", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "ApplyMotionTags"))
    TArray<FGameplayTag> mApplyMotionTags;

    /**
     * @brief 상대 방향으로 자동 회원 전환 여부
     */
    UPROPERTY(Category = "Motion", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AutoRotateTowardTarget"))
    bool mAutoRotateTowardTarget = true;
};

/**
* @brief 하나의 스킬 내에서 같은 타이밍에 처리하는 단위
* @details
* 발동될 하나의 애니메이션과 여러 효과를 묶어 "모션"이라는 단위로 정의
*/
USTRUCT(BlueprintType)
struct FSkillPhaseLayer
{
    GENERATED_BODY()

public:
    TArray<FTileIndex> FilterTileIndexes(const FTileIndex& SelfIndex, const TArray<FTileIndex>& TargetTileIndexes) const;
    TArray<IBoardCombatTarget*> FilterCombatTargets(const UTileMapModel* MapModel, const IBoardCombatTarget* SelfInstigator, const TArray<FTileIndex>& FilteredTileIndexes) const;

public:
    // @brief 하나의 모션 내에서 적용하는 단일 효과 단위의 TArray 묶음
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillEffectLayers"))
    TArray<TInstancedStruct<FSkillEffectLayer>> mSkillEffectLayers;

public:
    // @brief 자신, 지정 범위 포함 여부 타겟 필터링
    UPROPERTY(Category = "Filter", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETargetIndexFilter", DisplayName = "TargetIndexFilter"))
    int32 mTargetIndexFilter = static_cast<int32>(ETargetIndexFilter::IncludeTargetIndexes);

    // @brief 팀 타겟 필터링
    UPROPERTY(Category = "Filter", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETeamAttitudeFilter", DisplayName = "TeamAttitudeFilter"))
    int32 mTeamAttitudeFilter = static_cast<int32>(ETeamAttitudeFilter::Hostile);
};

/**
 * @brief  스킬 생성 시 사용되는 정적 Primary Data Asset
 */
UCLASS()
class P_RD_API UStaticSkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

    /* UPrimaryDataAsset 상속 */
public:
    FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(SkillPrimaryAssetTypes::GetObstacleActiveType(), GetFName());
    }

public:
    virtual FText MakeDescription() const;

#if WITH_EDITOR
public:
    EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

    /* UI 정보 */
public:
    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Name"))
    FText mName;

    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Description", MultiLine = true))
    FText mDescription;

    UPROPERTY(Category = "UI", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Icon", AssetBundles = "UI"))
    TSoftObjectPtr<UTexture2D> mIcon;
    
    /* 논리적 설정값들 */
public:
    // @brief 쿨다운시 적용될 Effect Class
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CooldownEffectClass", AssetBundles = "Actor"))
    TSoftClassPtr<UTacticalEffect_Cooldown> mCooldownEffectClass;

    // @brief 쿨다운까지 필요한 턴 수
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CooldownDuration"))
    int32 mCooldownDuration;

    // @brief 하나의 스킬 내에서 적용하는 단일 처리 단위의 TArray 묶음 (1개 : 단타, N개 : 연타)
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillPhaseLayers"))
    TArray<FSkillPhaseLayer> mSkillPhaseLayers;

public:
    // @brief 조준 범위 유형
    UPROPERTY(Category = "AimLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimPattern"))
    EAimPattern mAimPattern;

    // @brief 조준 가능 거리 계산 시 사용되는 기본 값 (Single 패턴은 자기 칸 조준이라 사용하지 않음)
    UPROPERTY(Category = "AimLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "AimRange", EditCondition = "mAimPattern != EAimPattern::Single", EditConditionHides))
    int32 mAimRange;

    // @brief 조준 시야를 막는 레이어 (비어 있으면 아무것도 조준을 막지 않음, Single 패턴은 사용하지 않음)
    UPROPERTY(Category = "AimLogic", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag", DisplayName = "AimBlockerMask", EditCondition = "mAimPattern != EAimPattern::Single", EditConditionHides))
    int32 mAimBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);

    // @brief 보드 액터도 조준 대상으로 지정 가능 여부 (Single 패턴은 자기 칸 조준이라 사용하지 않음)
    UPROPERTY(Category = "AimLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "CanAimBoardActor", EditCondition = "mAimPattern != EAimPattern::Single", EditConditionHides))
    bool mCanAimBoardActor = true;

public:
    // @brief 타겟 범위 유형
    UPROPERTY(Category = "TargetLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "TargetPattern"))
    ETargetPattern mTargetPattern = ETargetPattern::TargetOnly;

public:
    // @brief 영향 범위 유형
    UPROPERTY(Category = "EffectLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectPattern"))
    EEffectPattern mEffectPattern;
    
    // @brief 영향 범위 계산 시 사용되는 기본 값
    UPROPERTY(Category = "EffectLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "EffectArea"))
    int32 mEffectArea;

    // @brief 영향 확산을 막는 레이어 (비어 있으면 아무것도 확산을 막지 않음)
    UPROPERTY(Category = "EffectLogic", EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/P_RD.ETileLayerFlag", DisplayName = "EffectBlockerMask"))
    int32 mEffectBlockerMask = static_cast<int32>(ETileLayerFlag::Obstacle | ETileLayerFlag::Unit);

    /* 연출 설정값들 */
public:
    // @brief 스킬 실행 시 호출될 애니메이션 몽타쥬
    UPROPERTY(Category = "BaseLogic", EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "SkillAnimationSet"))
    FSkillAnimationSet mSkillAnimationSet;
};

