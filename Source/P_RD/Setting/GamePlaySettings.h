/*****************************************************************//**
 * @file   GamePlaySettings.h
 * @brief  게임 플레이 연관 설정 객체 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "Setting/UnitTeamType.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"

#include "GamePlaySettings.generated.h"

/**
 * @brief  게임 플레이 연관 설정
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Game Play Setting"))
class P_RD_API UGamePlaySettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
    UGamePlaySettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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
    UPROPERTY(Config, Category = UI, EditAnywhere, meta = (DisplayName = "WorldWidgetClasses", ArraySizeEnum = "EWorldWidgetType"))
    TSubclassOf<UUserWidget> mWorldWidgetClasses[static_cast<uint8>(EWorldWidgetType::Count)];

public:
    UPROPERTY(Config, Category = Title, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleRoomId"))
    FPrimaryAssetId mTitleRoomId;

    UPROPERTY(Config, Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "DefaultRoomMap", ConfigRestartRequired = true))
    TSoftObjectPtr<UWorld> mDefaultRoomMap;

public:
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "FrontendGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mFrontendGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mCombatGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ShopGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mShopGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TreasureGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mTreasureGameMode;

public:
    UPROPERTY(Config, Category = Team, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TeamRelations"))
    TMap<TEnumAsByte<EUnitTeamType::Type>, FUnitTeamRelation> mTeamRelations;
};
