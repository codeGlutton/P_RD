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

class UTexture2D;

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
    FString mIntroCinematicVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/Title/Video/Intro/H3_Intro_V87_Concept04_CastleEntry_16x9_15s.mp4");

    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "IntroCinematicAcceleratedVideoPath"))
    FString mIntroCinematicAcceleratedVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/Title/Video/Intro/H3_Intro_V87_Concept04_CastleEntry_16x9_15s_3x.mp4");

    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleBackgroundVideoPath"))
    FString mTitleBackgroundVideoPath = TEXT("SVN/OutSideAsset/AICreation/UI/Title/Video/Random30_Right16x9/Title_All6_Right_16x9_combo01_5s_mobile.mp4");

    /** @brief 타이틀에서 셔플 재생할 전체 화면 영상 목록. 비어 있으면 단일 TitleBackgroundVideoPath를 사용한다. */
    UPROPERTY(Config, Category = Media, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleBackgroundVideoPaths"))
    TArray<FString> mTitleBackgroundVideoPaths;

    /** @brief 타이틀 WBP의 TitleLogoImage에 런타임으로 적용할 SVN 텍스처. */
    UPROPERTY(Config, Category = UI, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "TitleLogoTexture"))
    TSoftObjectPtr<UTexture2D> mTitleLogoTexture;

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

    /* VFX 세팅 */
public:
    UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "GlobalStatusEffectVFXSetting", ToolTip = "상태이상 연출 시 사용되는 전역 VFX 설정 값"))
    FGlobalStatusEffectVFXSetting mGlobalStatusEffectVFXSetting;

public:
    UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetRemoveVFX", ToolTip = "전투 대상에서 타일 맵에서 제거될 때 호출되는 VFX 설정 값"))
    FSoftVFXSpawnData mCombatTargetRemoveVFX;
    
    UPROPERTY(Config, Category = VFX, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "CombatTargetVFXTimelineSettings", ToolTip = "전투 대상에서 활용되는 타임 라인 설정 값들"))
    TArray<FCombatTargetVFXTimelineSetting> mCombatTargetVFXTimelineSettings;
};

