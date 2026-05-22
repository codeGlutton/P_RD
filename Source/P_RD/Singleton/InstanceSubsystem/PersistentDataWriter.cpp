#include "Singleton/InstanceSubsystem/PersistentDataWriter.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"

UUserPersistData* IUserDataWriter::GetUserMutableData() const
{
	const UObject* Self = Cast<const UObject>(this);
	checkf(Self != nullptr, TEXT("언리얼 인터페이스는 반드시 UObject 상속"));

	UPersistentDataSubsystem* PersistentDataSubsystem = UGameplayStatics::GetGameInstance(Self)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->mUserPersistData;
}

URunPersistData* IRunDataWriter::GetRunMutableData() const
{
	const UObject* Self = Cast<const UObject>(this);
	checkf(Self != nullptr, TEXT("언리얼 인터페이스는 반드시 UObject 상속"));

	UPersistentDataSubsystem* PersistentDataSubsystem = UGameplayStatics::GetGameInstance(Self)->GetSubsystem<UPersistentDataSubsystem>();
	checkf(PersistentDataSubsystem != nullptr, TEXT("영구 데이터 서브시스템 nullptr"));

	return PersistentDataSubsystem->mRunPersistData;
}
