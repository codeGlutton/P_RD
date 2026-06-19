/*****************************************************************//**
 * @file   GameProfileSubsystem.h
 * @brief  게임 프로파일 관리를 위한 Subsystem 구현 헤더
 * @author 모호재
 * @date   2026-05-10
 *********************************************************************/

#pragma once

#include "RDMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentDataType.h"
#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"

#include "GameProfileSubsystem.generated.h"

 // Profile 신규 로그 카테고리 등록
DECLARE_LOG_CATEGORY_EXTERN(LogGameProfile, Log, All)

/**
 * @brief  게임 프로파일 관리를 위한 Subsystem
 */
UCLASS()
class P_RD_API UGameProfileSubsystem : public UGameInstanceSubsystem, public IUserDataWriter, public IRunDataWriter, public IOptionDataWriter
{
	GENERATED_BODY()

public:
	void MakeUser(const FText& Name) const;
	void StartRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty) const;
	void EndRun() const;

public:
	void SetVolume(EGameVolumeType VolumeType, float Volume) const;
	void SetLanguage(ELanguageType LanguageType) const;
	void SetResolution(const FIntPoint& Resolution) const;
	void ResetOptions() const;
};
