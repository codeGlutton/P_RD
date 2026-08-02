/*****************************************************************//**
 * @file   GamePlaySettings.h
 * @brief  게임 플레이 연관 설정 객체 정의 헤더
 * @author 모호재
 * @date   2026-04-27
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "ObjectModel.h"

#include "Setting/GameTeamType.h"
#include "Setting/ModelViewMapping.h"
#include "Setting/GamePlayType.h"
#include "Singleton/WorldSubsystem/WorldWidgetType.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"

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

    /* 월드 위젯 세팅 */
public:
    UPROPERTY(Config, Category = UI, EditAnywhere, meta = (DisplayName = "WorldWidgetClasses", ArraySizeEnum = "EWorldWidgetType"))
    TSubclassOf<UUserWidget> mWorldWidgetClasses[static_cast<uint8>(EWorldWidgetType::Count)];

    /* 모델-뷰 맵핑 세팅 */
public:
    UPROPERTY(Config, Category = Model, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "SubsystemModelViewMappings", ConfigRestartRequired = true))
    TSet<FSubsystemModelViewMapping> mSubsystemModelViewMappings;
    UPROPERTY(Config, Category = Model, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "WorldModelViewMappings", ConfigRestartRequired = true))
    TSet<FWorldModelViewMapping> mWorldModelViewMappings;

    /* 기본 룸 세팅 */
public:
    UPROPERTY(Config, Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "FrontendRoomId"))
    FPrimaryAssetId mFrontendRoomId;

    UPROPERTY(Config, Category = Room, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "DefaultBackgroundMap"))
    TSoftObjectPtr<UWorld> mDefaultBackgroundMap;

    /* 비디오 세팅 */
public:
    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IntroCinematicVideoPath"))
    FString mIntroCinematicVideoPath = TEXT("SVN/OutSideAsset/AICreation/hero_loading_intro4_1280_3s.mp4");

    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleBackgroundVideoPath"))
    FString mTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/campfire_titleloop_idle_x3preview.mp4");

    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatVictoryVideoPath"))
    FString mCombatVictoryVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Victory_01.mp4");

    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatDefeatVideoPath"))
    FString mCombatDefeatVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/CombatHUD/CombatResult/MS_CombatResult_Defeat_01.mp4");

    /* 룸 별 게임모드 세팅 */
public:
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "FrontendGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mFrontendGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mCombatGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "ShopGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mShopGameMode;
    UPROPERTY(Config, Category = GameMode, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TreasureGameMode", ConfigRestartRequired = true))
    TSoftClassPtr<AGameModeBase> mTreasureGameMode;

    /* 팀 세팅 */
public:
    UPROPERTY(Config, Category = Team, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TeamRelations"))
    TMap<TEnumAsByte<EGameTeamType::Type>, FGameTeamRelation> mTeamRelations;

    /* 사운드 세팅 */
public:
    UPROPERTY(Config, Category = Sound, EditAnywhere, meta = (DisplayName = "WorldWidgetClasses", ArraySizeEnum = "EGameVolumeType"))
    TSoftObjectPtr<USoundClass> mSoundClasses[static_cast<uint8>(EGameVolumeType::Count)];

    /* 상태이상 VFX 세팅 */
public:
    UPROPERTY(Config, Category = StatusEffect, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "GlobalStatusEffectVFXSetting", ToolTip = "상태이상 연출 시 사용되는 전역 VFX 설정 값"))
    FGlobalStatusEffectVFXSetting mGlobalStatusEffectVFXSetting;
};

