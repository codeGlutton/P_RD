#include "UI/Inventory/MockInventoryDriver.h"

#include "Engine/Texture2D.h"
#include "UI/Inventory/InventoryUIModel.h"
#include "UObject/ConstructorHelpers.h"

#if !UE_BUILD_SHIPPING

namespace
{
	struct FMockArtifactDefinition
	{
		const TCHAR* mName;
		const TCHAR* mIconPath;
	};

	const FMockArtifactDefinition MockArtifacts[] = {
		{
			TEXT("Ember Coin"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Treasure.T_MapNode_Treasure")
		},
		{
			TEXT("Moon Chalice"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_RareTreasure.T_MapNode_RareTreasure")
		},
		{
			TEXT("King's Reliquary"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_EpicTreasure.T_MapNode_EpicTreasure")
		},
		{
			TEXT("Arcane Compass"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_ArcaneShop.T_MapNode_ArcaneShop")
		},
		{
			TEXT("Witchglass"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_MagicShop.T_MapNode_MagicShop")
		},
		{
			TEXT("Merchant's Seal"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Shop.T_MapNode_Shop")
		},
		{
			TEXT("Spider Idol"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Monster.T_MapNode_Monster")
		},
		{
			TEXT("Hunter's Crest"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Elite.T_MapNode_Elite")
		},
		{
			TEXT("Broken Crown"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Boss.T_MapNode_Boss")
		},
		{
			TEXT("Warning Bell"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_Danger.T_MapNode_Danger")
		},
		{
			TEXT("Blood Pennant"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_EliteDanger.T_MapNode_EliteDanger")
		},
		{
			TEXT("Dragon's Omen"),
			TEXT("/Game/SVN/OutSideAsset/AICreation/UI/MapNode/T_MapNode_BossDanger.T_MapNode_BossDanger")
		},
	};

	FLinearColor MockRarityColor(int32 ArtifactIndex)
	{
		switch (ArtifactIndex % 3)
		{
		case 1:
			return FLinearColor(0.42f, 0.66f, 0.95f, 1.f);
		case 2:
			return FLinearColor(0.72f, 0.46f, 0.92f, 1.f);
		default:
			return FLinearColor(0.72f, 0.78f, 0.75f, 1.f);
		}
	}

	FText MockRarityLabel(int32 ArtifactIndex)
	{
		switch (ArtifactIndex % 3)
		{
		case 1:
			return NSLOCTEXT("MockInventoryDriver", "Rare", "RARE · PARTY-WIDE");
		case 2:
			return NSLOCTEXT("MockInventoryDriver", "Epic", "EPIC · PARTY-WIDE");
		default:
			return NSLOCTEXT("MockInventoryDriver", "Common", "COMMON · PARTY-WIDE");
		}
	}
}

#endif

UMockInventoryDriver::UMockInventoryDriver(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if !UE_BUILD_SHIPPING
	mPreviewIcons.Reserve(UE_ARRAY_COUNT(MockArtifacts));
	for (const FMockArtifactDefinition& Definition : MockArtifacts)
	{
		ConstructorHelpers::FObjectFinder<UTexture2D> IconFinder(
			Definition.mIconPath);
		mPreviewIcons.Add(IconFinder.Succeeded() ? IconFinder.Object : nullptr);
	}
#endif
}

void UMockInventoryDriver::Start(UInventoryUIModel* UIModel)
{
#if UE_BUILD_SHIPPING
	// QA fixture와 그 아이콘 하드 레퍼런스는 배포 빌드에 들어가지 않는다.
	return;
#else
	if (UIModel == nullptr)
	{
		return;
	}
	mUIModel = UIModel;

	FInventoryUI Inventory;
	Inventory.mGold = 1840;
	Inventory.mArtifacts.Reserve(UE_ARRAY_COUNT(MockArtifacts));

	for (int32 ArtifactIndex = 0;
		ArtifactIndex < UE_ARRAY_COUNT(MockArtifacts);
		++ArtifactIndex)
	{
		const FMockArtifactDefinition& Definition = MockArtifacts[ArtifactIndex];
		FInventoryArtifactUI& Artifact = Inventory.mArtifacts.AddDefaulted_GetRef();
		Artifact.mArtifactIndex = ArtifactIndex;
		Artifact.mName = FText::FromString(Definition.mName);
		Artifact.mIcon = mPreviewIcons.IsValidIndex(ArtifactIndex)
			? mPreviewIcons[ArtifactIndex]
			: nullptr;
		Artifact.mRarityColor = MockRarityColor(ArtifactIndex);
		Artifact.mDetailText = MockRarityLabel(ArtifactIndex);
	}

	// UIModel만 바꾼다. RunPersistData, PartyModel, SaveGame에는 접근하지 않는다.
	mUIModel->SetInventory(Inventory);
#endif
}
