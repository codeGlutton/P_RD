#include "P_RDEditorModule.h"
#include "PropertyEditorModule.h"
#include "DataAsset/SkillData/StaticSkillData.h"
#include "DataAsset/SkillData/StaticSkillDataPropertyCustomization.h"
#include "UI/MarchboundHireWidgetBuilder.h"
#include "UI/CombatHUDWidgetBuilder.h"
#include "UI/RewardSettlementWidgetBuilder.h"
#include "UI/RewardConceptBoardBuilder.h"
#include "UI/RewardConcept03NewWidgetBuilder.h"
#include "UI/CombatDefeatWidgetBuilder.h"
#include "UI/MonsterTabWidgetBuilder.h"
#include "UI/SettingsPanelWidgetBuilder.h"
#include "UI/WidgetVariableCleanup.h"
#include "UI/WidgetTexturePurge.h"
#include "UI/ShopWidgetBuilder.h"
#include "UI/ShopFullGeneratedWidgetBuilder.h"
#include "UI/WorldMapLandscapeWidgetBuilder.h"
#include "UI/SkillTacticalDiagramWidgetBuilder.h"

#include "ISequencerModule.h"
#include "Animation/Track/BoardEventTrackEditor.h"
#include "SequencerChannelInterface.h"
#include "Animation/Channel/BoardEventChannel.h"
#include "ICurveEditorModule.h"
#include "Animation/Channel/BoardEventChannelCurveModel.h"
#include "Animation/Channel/BoardEventChannelDetailsCustomization.h"
#include "Channels/SCurveEditorEventChannelView.h"

IMPLEMENT_GAME_MODULE(FP_RDEditorModule, P_RDEditor);

void FP_RDEditorModule::StartupModule()
{
	RegisterMarchboundHireWidgetBuilderCommands();
	RegisterCombatHUDWidgetBuilderCommands();
	RegisterRewardSettlementWidgetBuilderCommands();
	RegisterRewardConceptBoardBuilderCommands();
	RegisterRewardConcept03NewWidgetBuilderCommands();
	RegisterCombatDefeatWidgetBuilderCommands();
	RegisterMonsterTabWidgetBuilderCommands();
	RegisterSettingsPanelWidgetBuilderCommands();
	RegisterWidgetVariableCleanupCommands();
	RegisterWidgetTexturePurgeCommands();
	RegisterShopWidgetBuilderCommands();
	RegisterShopFullGeneratedWidgetBuilderCommands();
	RegisterWorldMapLandscapeWidgetBuilderCommands();
	RegisterSkillTacticalDiagramWidgetBuilderCommands();

	/* 커스텀 디테일 레이아웃 등록 */

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		PropertyModule.RegisterCustomClassLayout(
			UStaticSkillData::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FStaticSkillDataPropertyCustomization::MakeInstance)
		);
		PropertyModule.RegisterCustomPropertyTypeLayout(
			"BoardEventTriggerData", 
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FBoardEventChannelDetailsCustomization::MakeInstance)
		);
	}

	/* 보드 이벤트 트랙 등록 */

	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	if (FModuleManager::Get().IsModuleLoaded("Sequencer") == true)
	{
		mBoardEventTrackEditorHandle = SequencerModule.RegisterTrackEditor(
			FOnCreateTrackEditor::CreateStatic(&FBoardEventTrackEditor::CreateTrackEditor)
		);

		SequencerModule.RegisterChannelInterface<FBoardEventTriggerChannel>();
	}

	ICurveEditorModule& CurveEditorModule = FModuleManager::LoadModuleChecked<ICurveEditorModule>("CurveEditor");
	if (FModuleManager::Get().IsModuleLoaded("CurveEditor") == true)
	{
		FBoardEventChannelCurveModel::EventView = CurveEditorModule.RegisterView(FOnCreateCurveEditorView::CreateStatic(
			[](TWeakPtr<FCurveEditor> WeakCurveEditor) -> TSharedRef<SCurveEditorView>
			{
				return SNew(SCurveEditorEventChannelView, WeakCurveEditor);
			}
		));
	}
}

void FP_RDEditorModule::ShutdownModule()
{
	UnregisterMarchboundHireWidgetBuilderCommands();
	UnregisterCombatHUDWidgetBuilderCommands();
	UnregisterRewardSettlementWidgetBuilderCommands();
	UnregisterRewardConceptBoardBuilderCommands();
	UnregisterRewardConcept03NewWidgetBuilderCommands();
	UnregisterCombatDefeatWidgetBuilderCommands();
	UnregisterMonsterTabWidgetBuilderCommands();
	UnregisterSettingsPanelWidgetBuilderCommands();
	UnregisterWidgetVariableCleanupCommands();
	UnregisterWidgetTexturePurgeCommands();
	UnregisterShopWidgetBuilderCommands();
	UnregisterShopFullGeneratedWidgetBuilderCommands();
	UnregisterWorldMapLandscapeWidgetBuilderCommands();
	UnregisterSkillTacticalDiagramWidgetBuilderCommands();

	/* 커스텀 디테일 레이아웃 등록 해제 */

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor") == true)
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UStaticSkillData::StaticClass()->GetFName());
	}

	/* 보드 이벤트 트랙 등록 해제 */

	ISequencerModule& SequencerModule = FModuleManager::LoadModuleChecked<ISequencerModule>("Sequencer");
	if (FModuleManager::Get().IsModuleLoaded("Sequencer") == true)
	{
		SequencerModule.UnRegisterTrackEditor(
			mBoardEventTrackEditorHandle
		);
	}
}
