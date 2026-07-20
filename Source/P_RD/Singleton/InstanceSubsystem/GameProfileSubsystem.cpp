#include "Singleton/InstanceSubsystem/GameProfileSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

DEFINE_LOG_CATEGORY(LogGameProfile)

void UGameProfileSubsystem::MakeUser(const FText& Name) const
{
	GetRunMutableData()->ClearRun();
	GetUserMutableData()->MakeUser(Name);

	UE_LOG(LogGameProfile, Log, TEXT("새로운 유저 데이터 생성"));
}

void UGameProfileSubsystem::StartRun(const FPrimaryAssetId& PlayerUnitId, int32 Difficulty) const
{
	checkf(GetUserMutableData()->IsActive() == true, TEXT("유저 미존재 상태에서 새로운 런 생성 불가"));

	GetRunMutableData()->StartRun(PlayerUnitId, Difficulty);

	UE_LOG(LogGameProfile, Log, TEXT("새로운 런 데이터 생성"));
}

void UGameProfileSubsystem::EndRun() const
{
	UUserPersistData* UserMutableData = GetUserMutableData();
	URunPersistData* RunMutableData = GetRunMutableData();

	checkf(UserMutableData->IsActive() == true, TEXT("유저 미존재 상태에서 런 종료 불가"));
	checkf(RunMutableData->IsActive() == true, TEXT("런 미존재 상태에서 런 종료 불가"));

	UserMutableData->UpdateLog(RunMutableData->GetPlayerUnitId(), RunMutableData->GetRunLog());
	RunMutableData->ClearRun();

	UE_LOG(LogGameProfile, Log, TEXT("런 데이터 기록 후 삭제"));
}

void UGameProfileSubsystem::ClearCombatRoom(const FTileTransform& Transform) const
{
	URunPersistData* RunMutableData = GetRunMutableData();
	checkf(RunMutableData->IsActive() == true, TEXT("런 미존재 상태에서 전투 방 클리어 저장 불가"));

	RunMutableData->ClearCurrentCombatRoom(Transform);
}

void UGameProfileSubsystem::SetVolume(EGameVolumeType VolumeType, float Volume) const
{
	GetOptionMutableData()->SetVolume(VolumeType, Volume);

	UE_LOG(LogGameProfile, Log, TEXT("[%s] 볼륨 변경"), *EnumToString(VolumeType));
}

void UGameProfileSubsystem::SetLanguage(ELanguageType LanguageType) const
{
	GetOptionMutableData()->SetLanguage(LanguageType);

	UE_LOG(LogGameProfile, Log, TEXT("[%s] 언어 변경"), *EnumToString(LanguageType));
}

void UGameProfileSubsystem::SetOverallQuality(EOverallQualityType QualityType) const
{
	GetOptionMutableData()->SetOverallQuality(QualityType);

	UE_LOG(LogGameProfile, Log, TEXT("[%s] 퀄리티 변경"), *EnumToString(QualityType));
}

void UGameProfileSubsystem::SetFpsLimit(int32 FpsLimit) const
{
	GetOptionMutableData()->SetFpsLimit(FpsLimit);

	UE_LOG(LogGameProfile, Log, TEXT("[%d] FPS 제한 변경"), FpsLimit);
}

void UGameProfileSubsystem::SetCameraShakeEnabled(bool IsEnabled) const
{
	GetOptionMutableData()->SetCameraShakeEnabled(IsEnabled);

	UE_LOG(LogGameProfile, Log, TEXT("[%s] 카메라 흔들림 설정 변경"), (IsEnabled ? TEXT("True") : TEXT("False")));
}

void UGameProfileSubsystem::SetEffectVFXEnabled(bool IsEnabled) const
{
	GetOptionMutableData()->SetEffectVFXEnabled(IsEnabled);

	UE_LOG(LogGameProfile, Log, TEXT("[%s] 이펙트 VFX 설정 변경"), (IsEnabled ? TEXT("True") : TEXT("False")));
}

void UGameProfileSubsystem::ResetOptions() const
{
	GetOptionMutableData()->ClearOption();

	UE_LOG(LogGameProfile, Log, TEXT("옵션 초기화"));
}
