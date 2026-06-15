#include "GameMode/CombatGameMode.h"
#include "Singleton/WorldSubsystem/SRPGCombatSubsystem.h"

#include "Engine/AssetManager.h"
#include "Singleton/InstanceSubsystem/PersistentData.h"
#include "Singleton/WorldSubsystem/WorldWidgetSubsystem.h"
#include "DataAsset/RoomSpawnData/StaticCombatRoomSpawnData.h"

#include "PCGStage/Room.h"

#include "Pawn/Unit.h"
#include "UI/RDUserWidget.h"
#include "UI/CombatTileMapHUDWidget.h"
#include "UI/Combat/CombatViewModel.h"
#include "Dice/DiceAdapter.h"
#include "Combat/CombatViewAdapter.h"

#include "SRPGFramework/SRPGSkillAction.h"
#include "SRPGFramework/SRPGSkillBuildAction.h"

DEFINE_LOG_CATEGORY(LogCombatGameMode);

void ACombatGameMode::InitializeRoom()
{
	Super::InitializeRoom();

	const FRoom& CurRoom = GetRunPersistData()->GetCurrentRoom();

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	checkf(AssetManager != nullptr, TEXT("에셋 매니저 nullptr"));
	const UStaticCombatRoomSpawnData* StaticRoomData = AssetManager->GetPrimaryAssetObject<UStaticCombatRoomSpawnData>(CurRoom.mStaticRoomSpawnDataId);
	checkf(StaticRoomData != nullptr, TEXT("해당하는 룸 정보 탐색 실패"));

	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	CombatSubsystem->InitCombat(StaticRoomData, GetPlayerUnit());
}

void ACombatGameMode::BeginRoom()
{
	Super::BeginRoom();

	/*
	 * TopMenuBar는 RoomGameModeBase가 공용 월드 위젯으로 연다.
	 * 전투 타일맵 HUD는 전투방에만 필요한 화면이므로 CombatGameMode의 HUDClass로 분리해서 여기서 연다.
	 * 이렇게 해야 MAP/SET/DICE/SKILL 탑바 흐름과 전투 전용 액션 패널이 같은 WorldWidget 슬롯을 덮어쓰지 않는다.
	 */
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	/*
	 * 전투↔UI 경계 뷰모델은 전투를 굴리는 서브시스템이 소유한다.
	 * 여기서는 그 뷰모델을 꺼내 HUD와 주사위 어댑터에 "꽂기만" 한다(소유/배관은 한곳에서).
	 */
	UCombatViewModel* CombatViewModel = CombatSubsystem->GetCombatViewModel();

	/*
	 * 주사위 어댑터: 런 보유 주사위(mDiceIds)로 UDiceData를 만들고 뷰모델에 바인딩한다.
	 * HUD가 열릴 때 뷰모델에 주사위 뷰가 이미 있어야 하므로 HUD 열기 "전"에 꽂는다.
	 * 이제 HUD의 RequestRollDice() → 어댑터가 데이터로 굴림 → SetDiceViews → HUD가 실제 값을 읽는다.
	 * (UI의 FMath::RandRange fallback은 뷰모델 미연결 때만 동작하므로 자연히 비활성화)
	 */
	const URunPersistData* RunPersistData = GetRunPersistData();

	mDiceAdapter = NewObject<UDiceAdapter>(this);
	if (RunPersistData != nullptr)
	{
		mDiceAdapter->BuildFromDiceIds(RunPersistData->GetDiceIds());
	}
	mDiceAdapter->BindViewModel(CombatViewModel);

	/*
	 * 유닛/메타/턴 표시 어댑터(비GAS). 스폰된 유닛·런 데이터로 표시값을 만들어 같은 뷰모델에 push.
	 * → 전투 HUD 상단 상태바(HP/Gold/Lv/Round)와 이후 유닛 HP바가 이 값을 읽는다.
	 */
	mCombatViewAdapter = NewObject<UCombatViewAdapter>(this);
	mCombatViewAdapter->Build(CombatSubsystem, RunPersistData);
	mCombatViewAdapter->BindViewModel(CombatViewModel);

	if (UWorldWidgetSubsystem* WorldWidgetSubsystem = GetWorld()->GetSubsystem<UWorldWidgetSubsystem>())
	{
		if (UCombatTileMapHUDWidget* CombatHUD = WorldWidgetSubsystem->GetHUD<UCombatTileMapHUDWidget>())
		{
			CombatHUD->BindCombatViewModel(CombatViewModel);
			CombatHUD->OpenUI();
		}
	}

	for (const TObjectPtr<AUnit>& Unit : CombatSubsystem->GetUnits())
	{
		Unit->OnBeginRoom();
	}
	CombatSubsystem->BeginCombat();
}

bool ACombatGameMode::SelectSkill(int32 SkillIndex)
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGSkillSelectCommand> SkillSelectCommand = MakeShared<FSRPGSkillSelectCommand>();
	SkillSelectCommand->mSkillIndex = SkillIndex;
	SkillSelectCommand->OnChangeSkillBuildPhase.AddUObject(this, &ACombatGameMode::OnChangeSkillBuildPhase);

	return CombatSubsystem->SummitCommand(SkillSelectCommand);
}

bool ACombatGameMode::ResolveWorldTouchEvent()
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGWorldTraceCommand> WorldTraceActionCommand = MakeShared<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand->mIsLongPress = false;

	return CombatSubsystem->SummitCommand(WorldTraceActionCommand);
}

bool ACombatGameMode::ResolveWorldLongPressEvent()
{
	USRPGCombatSubsystem* CombatSubsystem = GetWorld()->GetSubsystem<USRPGCombatSubsystem>();
	checkf(CombatSubsystem != nullptr, TEXT("전투 시스템 서브시스템 nullptr"));

	TSharedPtr<FSRPGWorldTraceCommand> WorldTraceActionCommand = MakeShared<FSRPGWorldTraceCommand>();
	WorldTraceActionCommand->mIsLongPress = true;

	return CombatSubsystem->SummitCommand(WorldTraceActionCommand);
}

void ACombatGameMode::OnChangeSkillBuildPhase(const FSRPGSkillBuildAction& Action, ESRPGSkillBuildPhase Phase)
{
}
