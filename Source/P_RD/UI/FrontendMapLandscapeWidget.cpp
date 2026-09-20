#include "UI/FrontendMapLandscapeWidget.h"

#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

#if WITH_EDITOR
void UFrontendMapLandscapeWidget::SetLandscapeGraphWidgetClassesForEditor(
	TSubclassOf<UFrontendMapLineWidget> InLineWidgetClass,
	TSubclassOf<UFrontendMapNodeWidget> InNodeWidgetClass)
{
	SetMapGraphWidgetClasses(InLineWidgetClass, InNodeWidgetClass);
}
#endif

UFrontendMapLandscapeLineWidget::UFrontendMapLandscapeLineWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UTexture2D> SolidFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
			"T_WorldMap_Path_Gold_Gen_20260812.T_WorldMap_Path_Gold_Gen_20260812"));
	static ConstructorHelpers::FObjectFinder<UTexture2D> DashedFinder(
		TEXT("/Game/SVN/OutSideAsset/AICreation/UI/RunFlow/"
			"T_MapPath_Locked_V2.T_MapPath_Locked_V2"));
	SetLineTexturesForLayout(
		SolidFinder.Succeeded() ? SolidFinder.Object : nullptr,
		DashedFinder.Succeeded() ? DashedFinder.Object : nullptr);
	SetLineThicknessForLayout(20.f);
	SetLinePaletteForLayout(
		FLinearColor(1.00f, 0.70f, 0.20f, 1.00f),
		FLinearColor(1.00f, 0.91f, 0.55f, 1.00f),
		FLinearColor(0.76f, 0.57f, 0.27f, 0.74f),
		FLinearColor(1.00f, 0.43f, 0.04f, 0.52f),
		FLinearColor(0.78f, 0.40f, 0.08f, 0.24f));
}

UFrontendMapLandscapeNodeWidget::UFrontendMapLandscapeNodeWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	auto LoadTexture = [](const TCHAR* Path) -> UTexture2D*
	{
		return LoadObject<UTexture2D>(nullptr, Path);
	};

	mLandscapeMonster = LoadTexture(TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
		"T_WorldMap_Node_Battle_Gen_20260812.T_WorldMap_Node_Battle_Gen_20260812"));
	mLandscapeElite = LoadTexture(TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
		"T_WorldMap_Node_Elite_Gen_20260812.T_WorldMap_Node_Elite_Gen_20260812"));
	mLandscapeBoss = LoadTexture(TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
		"T_WorldMap_Node_Boss_Gen_20260812.T_WorldMap_Node_Boss_Gen_20260812"));
	mLandscapeShop = LoadTexture(TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
		"T_WorldMap_Node_Shop_Gen_20260812.T_WorldMap_Node_Shop_Gen_20260812"));
	mLandscapeTreasure = LoadTexture(TEXT(
		"/Game/SVN/OutSideAsset/AICreation/UI/P_RD/WorldMap/"
		"T_WorldMap_Node_Treasure_Gen_20260812.T_WorldMap_Node_Treasure_Gen_20260812"));
}

UTexture2D* UFrontendMapLandscapeNodeWidget::GetTypeIconTexture(ERoomType RoomType) const
{
	switch (RoomType)
	{
	case ERoomType::Monster:      return mLandscapeMonster;
	case ERoomType::EliteMonster: return mLandscapeElite;
	case ERoomType::BossMonster:  return mLandscapeBoss;
	case ERoomType::Shop:         return mLandscapeShop;
	case ERoomType::Treasure:     return mLandscapeTreasure;
	default:                      return Super::GetTypeIconTexture(RoomType);
	}
}

UTexture2D* UFrontendMapLandscapeNodeWidget::GetStateRingTexture(
	EMapRoomState RoomState, bool bIsCurrentRoom) const
{
	// 새 노드 파츠는 금속 외곽 링까지 한 장에 포함한다. 기존 상태 링을 겹치면
	// 이중 테두리가 생기므로 잠김은 아이콘 틴트, 현재 위치는 전용 파티 마커로 표현한다.
	(void)RoomState;
	(void)bIsCurrentRoom;
	return nullptr;
}

float UFrontendMapLandscapeNodeWidget::GetVisualScale(
	ERoomType RoomType, EMapRoomState RoomState, bool bIsCurrentRoom) const
{
	if (RoomType == ERoomType::BossMonster)
	{
		return 1.75f;
	}
	if (bIsCurrentRoom)
	{
		return 1.14f;
	}
	if (RoomState == EMapRoomState::Selected)
	{
		return 1.10f;
	}
	return 1.f;
}
