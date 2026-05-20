#include "GameMode/RoomGameModeBase.h"
#include "Singleton/InstanceSubsystem/PersistentDataSubsystem.h"

void ARoomGameModeBase::BeginPlay()
{
	Super::BeginPlay();



	// 방 이동 기록 및 저장
	// 
	// 메인 유닛 생성
	// GAS 컴포넌트 로드
	// 인벤토리 컴포넌트 로드
	// 
	// 방 무관계 UI 생성
	// Subsystem Init 타이밍
	// + 메인 유닛 등록
	// + 방 타입 연관 유닛 생성
	// + 방 타입 연관 UI 생성

	//UPersistentDataSubsystem* PersistentDataSubsystem = GetGameInstance()->GetSubsystem<UPersistentDataSubsystem>();
	//
	//PersistentDataSubsystem->GetRunPersistData()->MakeNewRun(1);
	//PersistentDataSubsystem->GetRunPersistData()->MakeStageAsync(EStageLevelType::Stage1, FOnCreateStage());
}

AUnit* ARoomGameModeBase::GetPlayerUnit() const
{
	return mPlayerUnit.Get();
}