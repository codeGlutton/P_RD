/*****************************************************************//**
 * @file   UnitTeamSettings.h
 * @brief  팀 관계도 설정에 대한 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "GenericTeamAgentInterface.h"
#include "Setting/UnitTeamType.h"

#include "UnitTeamSettings.generated.h"

/**
 * @brief  한 팀에서 다른 팀들을 대하는 관계 정보 구조체
 */
USTRUCT(BlueprintType)
struct FUnitTeamRelation
{
    GENERATED_BODY()

public:
    FUnitTeamRelation();
    FUnitTeamRelation(std::initializer_list<TPairInitializer<const TEnumAsByte<EUnitTeamType::Type>&, const TEnumAsByte<ETeamAttitude::Type>&>> Attitudes) : mAttitudes(MoveTemp(Attitudes)) {}

public:
    UPROPERTY(Category = Team, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Attitudes"))
    TMap<TEnumAsByte<EUnitTeamType::Type>, TEnumAsByte<ETeamAttitude::Type>> mAttitudes;
};

/**
 * @brief  팀 관계도 설정
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Unit Team Setting"))
class P_RD_API UUnitTeamSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
    UUnitTeamSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /* UDeveloperSettings 상속 */
public:
    FName GetCategoryName() const override;

#if WITH_EDITOR
    FText GetSectionText() const override;
    FText GetSectionDescription() const override;
#endif

public:
    /**
     * 설정된 Team Relation을 통해 Team Attribute를 결정하는 정적 함수.
     * 팀을 결정하는 기본 Solver 함수로 사용
     * @param OwnId 자신의 팀 ID
     * @param OtherId 대상의 팀 ID
     * @return 대상에 대한 자신의 Attribute 값
     */
    UFUNCTION(Category = Team, BlueprintPure)
    static ETeamAttitude::Type GetAttitude(FGenericTeamId OwnId, FGenericTeamId OtherId);

public:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = Team, meta = (DisplayName = "TeamRelations"))
    TMap<TEnumAsByte<EUnitTeamType::Type>, FUnitTeamRelation> mTeamRelations;
};
