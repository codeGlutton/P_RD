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

	URunPersistData* RunMutableData = GetRunMutableData();
	RunMutableData->StartRun(PlayerUnitId, Difficulty);
	RunMutableData->SetTutorialEnabled(GetUserMutableData()->IsTutorialCompleted() == false);

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

void UGameProfileSubsystem::SetResolution(const FIntPoint& Resolution) const
{
	GetOptionMutableData()->SetResolution(Resolution);

	UE_LOG(LogGameProfile, Log, TEXT("[%d x %d] 해상도 변경"), Resolution.X, Resolution.Y);
}

void UGameProfileSubsystem::SetRenderResolution(int32 ShortSideHeight) const
{
	GetOptionMutableData()->SetRenderResolution(ShortSideHeight);

	UE_LOG(LogGameProfile, Log, TEXT("[%dp] 렌더 해상도 변경"), ShortSideHeight);
}

void UGameProfileSubsystem::SetFpsLimit(int32 FpsLimit) const
{
	GetOptionMutableData()->SetFpsLimit(FpsLimit);

	UE_LOG(LogGameProfile, Log, TEXT("[%d] FPS 제한 변경"), FpsLimit);
}

void UGameProfileSubsystem::ResetOptions() const
{
	GetOptionMutableData()->ClearOption();

	UE_LOG(LogGameProfile, Log, TEXT("옵션 초기화"));
}
