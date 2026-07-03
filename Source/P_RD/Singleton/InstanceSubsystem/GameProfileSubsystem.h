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

	/** @brief FPS 상한(30/60) 저장+즉시 적용. */
	void SetFpsLimit(int32 FpsLimit) const;

	/** @brief 그래픽 품질(0~2) 저장+Scalability 즉시 적용. */
	void SetQualityLevel(int32 QualityLevel) const;

	/** @brief 화면 흔들림 선호 저장(소비자는 IsScreenShakeEnabled 조회). */
	void SetScreenShakeEnabled(bool bEnabled) const;

	/** @brief 전투 이펙트 표시 선호 저장(소비자는 AreEffectsEnabled 조회). */
	void SetEffectsEnabled(bool bEnabled) const;

	/** @brief 옵션 읽기 전용 접근자. UI 등 외부는 이걸로 조회하고, 쓰기는 위 Set* 래퍼로만 한다. */
	const UOptionPersistData* GetOptionData() const;
};
