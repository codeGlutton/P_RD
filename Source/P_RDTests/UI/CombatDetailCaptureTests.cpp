/*****************************************************************//**
 * @file   CombatDetailCaptureTests.cpp
 * @brief  실제 전투 HUD의 용병/몬스터/스킬/아티팩트 상세를 PNG로 남긴다.
 * @date   2026-08-18
 *********************************************************************/

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/SkeletalMeshActor.h"
#include "Actor/TileMap/TileMap.h"
#include "Actor/TileMap/TileMapModel.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "Slate/WidgetRenderer.h"
#include "TextureCompiler.h"
#include "UI/Combat/CombatLayoutHUDWidget.h"
#include "UI/Combat/CombatUIModel.h"
#include "UI/Combat/SkillTacticalDiagramWidget.h"
#include "Widgets/Colors/SColorBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/SOverlay.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

namespace CombatDetailCaptureTests
{
	constexpr int32 DesignWidth = 1920;
	constexpr int32 DesignHeight = 1080;
	constexpr int32 CaptureWidth = 1672;
	constexpr int32 CaptureHeight = 941;
	constexpr TCHAR HUDClassPath[] =
		TEXT("/Game/UI/CombatLayouts/WBP_CombatHUD04.WBP_CombatHUD04_C");

	FString OutputDirectory()
	{
		return FPaths::Combine(FPaths::ProjectSavedDir(),
			TEXT("UI"), TEXT("CombatDetails"));
	}

	UTexture2D* LoadTexture(const TCHAR* ObjectPath)
	{
		return LoadObject<UTexture2D>(nullptr, ObjectPath);
	}

	TSharedRef<SWidget> ScaleToCapture(const TSharedRef<SWidget>& Inner)
	{
		return SNew(SScaleBox)
			.Stretch(EStretch::UserSpecified)
			.UserSpecifiedScale(float(CaptureHeight) / float(DesignHeight))
			[
				SNew(SBox)
				.WidthOverride(float(DesignWidth))
				.HeightOverride(float(DesignHeight))
				[
					Inner
				]
			];
	}

	int32 MakeTexturesResident(UUserWidget* UserWidget)
	{
		if (UserWidget == nullptr || UserWidget->WidgetTree == nullptr)
		{
			return 0;
		}

		TArray<UWidget*> Widgets;
		UserWidget->WidgetTree->GetAllWidgets(Widgets);
		int32 Count = 0;
		auto MakeBrushResident = [&Count](const FSlateBrush& Brush)
		{
			UTexture2D* Texture = Cast<UTexture2D>(Brush.GetResourceObject());
			if (Texture == nullptr)
			{
				return;
			}
			Texture->UpdateResource();
			Texture->SetForceMipLevelsToBeResident(30.f);
			Texture->WaitForStreaming();
			++Count;
		};

		for (UWidget* Widget : Widgets)
		{
			if (const UImage* Image = Cast<UImage>(Widget))
			{
				MakeBrushResident(Image->GetBrush());
			}
			else if (const UBorder* Border = Cast<UBorder>(Widget))
			{
				MakeBrushResident(Border->Background);
			}
			else if (const UProgressBar* Bar = Cast<UProgressBar>(Widget))
			{
				MakeBrushResident(Bar->GetWidgetStyle().BackgroundImage);
				MakeBrushResident(Bar->GetWidgetStyle().FillImage);
			}
			else if (const UButton* Button = Cast<UButton>(Widget))
			{
				MakeBrushResident(Button->GetStyle().Normal);
				MakeBrushResident(Button->GetStyle().Hovered);
				MakeBrushResident(Button->GetStyle().Pressed);
				MakeBrushResident(Button->GetStyle().Disabled);
			}
			if (UUserWidget* Nested = Cast<UUserWidget>(Widget))
			{
				Count += MakeTexturesResident(Nested);
			}
		}
		return Count;
	}

	bool Capture(UCombatLayoutHUDWidget& HUD, UUserWidget* Foreground,
		const TCHAR* FileName, FString& OutError)
	{
		HUD.SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		HUD.ForceLayoutPrepass();
		if (Foreground != nullptr)
		{
			Foreground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Foreground->ForceLayoutPrepass();
		}

		const TSharedRef<SOverlay> Root = SNew(SOverlay);
		Root->AddSlot()
		[
			SNew(SColorBlock).Color(FLinearColor(.008f, .009f, .011f, 1.f))
		];
		Root->AddSlot()
		[
			ScaleToCapture(HUD.TakeWidget())
		];
		if (Foreground != nullptr)
		{
			Root->AddSlot()
			[
				ScaleToCapture(Foreground->TakeWidget())
			];
		}

		FTextureCompilingManager::Get().FinishAllCompilation();
		const int32 TextureCount = MakeTexturesResident(&HUD)
			+ MakeTexturesResident(Foreground);
		if (TextureCount == 0)
		{
			OutError = TEXT("상세 캡처에서 상주시킬 브러시 텍스처가 없음");
			return false;
		}
		FlushRenderingCommands();

		FWidgetRenderer Renderer(true, true);
		Renderer.SetIsPrepassNeeded(true);
		UTextureRenderTarget2D* RenderTarget = Renderer.DrawWidget(
			Root, FVector2D(CaptureWidth, CaptureHeight));
		if (RenderTarget == nullptr)
		{
			OutError = TEXT("상세 캡처 렌더 타깃 생성 실패");
			return false;
		}

		FlushRenderingCommands();
		TArray<FColor> Pixels;
		FReadSurfaceDataFlags ReadFlags(RCM_UNorm);
		ReadFlags.SetLinearToGamma(false);
		if (!RenderTarget->GameThread_GetRenderTargetResource()->ReadPixels(
			Pixels, ReadFlags)
			|| Pixels.Num() != CaptureWidth * CaptureHeight)
		{
			OutError = TEXT("상세 캡처 픽셀 읽기 실패");
			return false;
		}

		// CombatLayout 캡처와 동일한 RGBA8 이중 감마 보정.
		for (FColor& Pixel : Pixels)
		{
			Pixel.R = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.R / 255.f, 2.2f)));
			Pixel.G = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.G / 255.f, 2.2f)));
			Pixel.B = uint8(FMath::RoundToInt(
				255.f * FMath::Pow(Pixel.B / 255.f, 2.2f)));
		}

		uint8 MinChannel = 255;
		uint8 MaxChannel = 0;
		for (const FColor& Pixel : Pixels)
		{
			MinChannel = FMath::Min3(MinChannel, Pixel.R,
				FMath::Min(Pixel.G, Pixel.B));
			MaxChannel = FMath::Max3(MaxChannel, Pixel.R,
				FMath::Max(Pixel.G, Pixel.B));
		}
		if (int32(MaxChannel) - int32(MinChannel) < 8)
		{
			OutError = TEXT("상세 캡처가 단색임");
			return false;
		}

		const FString OutputPath = FPaths::Combine(OutputDirectory(), FileName);
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(
			CaptureWidth, CaptureHeight, Pixels, Png);
		if (!FFileHelper::SaveArrayToFile(Png, *OutputPath))
		{
			OutError = FString::Printf(TEXT("상세 PNG 저장 실패: %s"), *OutputPath);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[CombatDetailCapture] %s (%d textures)"),
			*OutputPath, TextureCount);
		return true;
	}

	struct FDetailFixture
	{
		TObjectPtr<UCombatLayoutHUDWidget> HUD;
		TObjectPtr<UCombatUIModel> Model;
		FUnitUI Knight;
		FUnitUI Mage;
		FUnitUI Rogue;
		FUnitUI Spider;
		FUnitUI Golem;
		FUnitDetailUI KnightDetail;
		FUnitDetailUI SpiderDetail;
		FSkillDetailUI SkillDetail;
		FPlayerMetaUI Meta;
	};

	FUnitDetailSkillUI SkillSlot(const int32 Index, const TCHAR* Name,
		UTexture2D* Icon)
	{
		FUnitDetailSkillUI Skill;
		Skill.mSkillIndex = Index;
		Skill.mName = FText::FromString(Name);
		Skill.mIcon = Icon;
		return Skill;
	}

	FDetailFixture MakeFixture(UWorld& World)
	{
		FDetailFixture Fixture;
		UClass* HUDClass = LoadClass<UCombatLayoutHUDWidget>(nullptr, HUDClassPath);
		Fixture.HUD = HUDClass != nullptr
			? CreateWidget<UCombatLayoutHUDWidget>(&World, HUDClass) : nullptr;
		if (Fixture.HUD == nullptr)
		{
			return Fixture;
		}
		Fixture.HUD->TakeWidget();
		Fixture.Model = NewObject<UCombatUIModel>(Fixture.HUD);
		Fixture.HUD->BindUIModel(Fixture.Model);

		UTexture2D* KnightPortrait = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
			"T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		UTexture2D* MagePortrait = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
			"T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		UTexture2D* RoguePortrait = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/"
			"T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		UTexture2D* SpiderPortrait = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Portraits/"
			"KK_Face_Enemy_Spider_ActionV3.KK_Face_Enemy_Spider_ActionV3"));
		UTexture2D* GolemPortrait = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Portraits/"
			"KK_Face_Enemy_Golem_ActionV3.KK_Face_Enemy_Golem_ActionV3"));
		UTexture2D* Whirlwind = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/"
			"T_SkillIcon_Whirlwind.T_SkillIcon_Whirlwind"));
		UTexture2D* Barrier = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/"
			"T_SkillIcon_Barrier.T_SkillIcon_Barrier"));
		UTexture2D* Charge = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/"
			"T_SkillIcon_Charge.T_SkillIcon_Charge"));
		UTexture2D* BeastClaw = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/CombatHUD/SkillIcons/"
			"T_SkillIcon_BeastClaw.T_SkillIcon_BeastClaw"));

		auto MakeUnit = [](const int32 Id, const bool bPlayer, const TCHAR* Name,
			const int32 Level, UTexture2D* Portrait, const float HP,
			const float MaxHP, const int32 AP, const int32 MaxAP,
			const float Speed) -> FUnitUI
		{
			FUnitUI Unit;
			Unit.mUnitId = Id;
			Unit.mIsPlayer = bPlayer;
			Unit.mName = FText::FromString(Name);
			Unit.mLevel = Level;
			Unit.mPortrait = Portrait;
			Unit.mTurnPortrait = Portrait;
			Unit.mHP = HP;
			Unit.mMaxHP = MaxHP;
			Unit.mActionPoints = AP;
			Unit.mMaxActionPoints = MaxAP;
			Unit.mMovementPoint = float(AP);
			Unit.mMaxMovementPoint = float(MaxAP);
			Unit.mSpeedPoint = Speed;
			Unit.mDamagePoint = 18.f + Level;
			Unit.mDefensePoint = 9.f + Level;
			Unit.mSkillPoint = 13.f + Level;
			return Unit;
		};

		Fixture.Knight = MakeUnit(101, true, TEXT("왕국 기사 로웬"), 8,
			KnightPortrait, 86.f, 100.f, 7, 10, 14.f);
		Fixture.Mage = MakeUnit(102, true, TEXT("별빛 마도사 리아"), 7,
			MagePortrait, 61.f, 72.f, 8, 11, 17.f);
		Fixture.Rogue = MakeUnit(103, true, TEXT("그림자 도적 카일"), 7,
			RoguePortrait, 68.f, 80.f, 9, 12, 21.f);
		Fixture.Spider = MakeUnit(901, false, TEXT("부패한 동굴거미"), 9,
			SpiderPortrait, 128.f, 150.f, 6, 8, 16.f);
		Fixture.Golem = MakeUnit(902, false, TEXT("고대 숲 골렘"), 11,
			GolemPortrait, 220.f, 240.f, 4, 7, 8.f);
		Fixture.Spider.mNextSkillIcon = BeastClaw;
		Fixture.Spider.mNextSkillIndex = 0;
		Fixture.Golem.mNextSkillIcon = Charge;
		Fixture.Golem.mNextSkillIndex = 0;

		Fixture.KnightDetail.mUnitId = Fixture.Knight.mUnitId;
		Fixture.KnightDetail.mName = Fixture.Knight.mName;
		Fixture.KnightDetail.mLevel = Fixture.Knight.mLevel;
		Fixture.KnightDetail.mPortrait = KnightPortrait;
		Fixture.KnightDetail.mPassiveDescriptions = {
			FText::FromString(TEXT("수호 태세: 받는 피해가 10% 감소합니다.")),
			FText::FromString(TEXT("전열 지휘: 인접한 아군의 방어력이 증가합니다.")) };
		Fixture.KnightDetail.mSkills = {
			SkillSlot(0, TEXT("방패 강타"), Barrier),
			SkillSlot(1, TEXT("돌격"), Charge),
			SkillSlot(2, TEXT("칼날 폭풍"), Whirlwind) };

		Fixture.SpiderDetail.mUnitId = Fixture.Spider.mUnitId;
		Fixture.SpiderDetail.mName = Fixture.Spider.mName;
		Fixture.SpiderDetail.mLevel = Fixture.Spider.mLevel;
		Fixture.SpiderDetail.mPortrait = SpiderPortrait;
		Fixture.SpiderDetail.mPassiveDescriptions = {
			FText::FromString(TEXT("독성 갑각: 공격받으면 공격자에게 독을 부여합니다.")),
			FText::FromString(TEXT("거미줄 사냥: 이동력이 낮은 적에게 추가 피해를 줍니다.")) };
		Fixture.SpiderDetail.mSkills = {
			SkillSlot(0, TEXT("맹독 이빨"), BeastClaw),
			SkillSlot(1, TEXT("그물 투척"), Barrier) };

		Fixture.SkillDetail.mSkillIndex = 2;
		Fixture.SkillDetail.mName = FText::FromString(TEXT("칼날 폭풍"));
		Fixture.SkillDetail.mDescription = FText::FromString(TEXT(
			"검을 크게 휘둘러 주변의 모든 적을 연속으로 공격합니다.\n"
			"중심에 가까운 적은 더 큰 피해를 받습니다."));
		Fixture.SkillDetail.mIcon = Whirlwind;
		Fixture.SkillDetail.mActionPointCost = 4;
		Fixture.SkillDetail.mActionPointGain = 1;
		Fixture.SkillDetail.mCooldownTurns = 2;
		Fixture.SkillDetail.mDamageMin = 24;
		Fixture.SkillDetail.mDamageMax = 32;
		Fixture.SkillDetail.mCriticalDamage = 48;
		Fixture.SkillDetail.mTargeting.mSelectShape =
			ECombatSkillSelectShapeUI::Cross;
		Fixture.SkillDetail.mTargeting.mSelectRange = 2.f;
		Fixture.SkillDetail.mTargeting.mHitShape =
			ECombatSkillHitShapeUI::Circle;
		Fixture.SkillDetail.mTargeting.mHitRange = 2.f;
		Fixture.SkillDetail.mTargeting.mAimBlockerMask = 0;
		Fixture.SkillDetail.mTargeting.mEffectBlockerMask = 0;

		Fixture.Meta.mGold = 840;
		Fixture.Meta.mLevel = 8;
		Fixture.Meta.mExp = 215.f;
		Fixture.Meta.mMaxExp = 300.f;
		FCombatArtifactUI& Artifact = Fixture.Meta.mArtifacts.AddDefaulted_GetRef();
		Artifact.mName = FText::FromString(TEXT("피의 성배"));
		Artifact.mIcon = LoadTexture(TEXT(
			"/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/"
			"T_Artifact_BloodChalice.T_Artifact_BloodChalice"));
		Artifact.mRarityName = FText::FromString(TEXT("영웅 아티팩트"));
		Artifact.mRarityLevel = 2;
		Artifact.mPrice = 350;
		Artifact.mRarityColor = FLinearColor(.75f, .28f, 1.f, 1.f);
		Artifact.mEffectDescriptions = {
			FText::FromString(TEXT("적을 처치하면 모든 아군의 HP를 5 회복합니다.")),
			FText::FromString(TEXT("HP가 절반 이하일 때 공격력이 12% 증가합니다.")) };

		Fixture.Model->SetUnitUIs({ Fixture.Knight, Fixture.Mage,
			Fixture.Rogue, Fixture.Spider, Fixture.Golem });
		Fixture.Model->SetPlayerMeta(Fixture.Meta);
		FTurnUI Turn;
		Turn.mCurrentUnitId = Fixture.Knight.mUnitId;
		Turn.mRound = 4;
		Turn.mPhase = ECombatBuildPhaseUI::SkillSelected;
		Turn.mTurnOrderUnitIds = { Fixture.Knight.mUnitId,
			Fixture.Spider.mUnitId, Fixture.Mage.mUnitId,
			Fixture.Golem.mUnitId, Fixture.Rogue.mUnitId };
		Turn.mCurrentRoundRemainingTurnCount = Turn.mTurnOrderUnitIds.Num();
		Fixture.Model->SetTurnUI(Turn);
		return Fixture;
	}

	/**
	 * 스킬 상세의 RenderTarget 검수용 작은 실제 전장. 그림을 합성하지 않고
	 * 프로젝트가 인게임에서 쓰는 ATileMap/하이라이트 머티리얼과 KayKit 스켈레탈
	 * 메시를 그대로 SceneCapture2D가 렌더한다.
	 */
	struct FWorldPreviewScene
	{
		TArray<TWeakObjectPtr<AActor>> Actors;

		void Destroy()
		{
			for (const TWeakObjectPtr<AActor>& Actor : Actors)
			{
				if (Actor.IsValid())
				{
					Actor->Destroy();
				}
			}
			Actors.Reset();
		}
	};

	FWorldPreviewScene BuildWorldPreviewScene(UWorld& World)
	{
		FWorldPreviewScene Scene;
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto Remember = [&Scene](AActor* Actor)
		{
			if (Actor != nullptr)
			{
				Scene.Actors.Add(Actor);
			}
			return Actor;
		};

		// 바닥은 실제 던전 재질을 쓴다. 그 위에 ATileMap의 반투명 타일과
		// Aim/Select/Effect 색이 합성되는 순서는 실전과 완전히 같다.
		AStaticMeshActor* Floor = Cast<AStaticMeshActor>(Remember(
			World.SpawnActor<AStaticMeshActor>(FVector(875.f, 625.f, -55.f),
				FRotator::ZeroRotator, Params)));
		if (Floor != nullptr)
		{
			UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr,
				TEXT("/Engine/BasicShapes/Plane.Plane"));
			UMaterialInterface* DungeonFloor = LoadObject<UMaterialInterface>(nullptr,
				TEXT("/Game/SVN/OutSideAsset/Background/Fantastic_Dungeon_Pack/"
					"materials/MI_MOD_Floor_01_v1.MI_MOD_Floor_01_v1"));
			Floor->GetStaticMeshComponent()->SetStaticMesh(Plane);
			Floor->GetStaticMeshComponent()->SetMaterial(0, DungeonFloor);
			Floor->SetActorScale3D(FVector(24.f, 19.f, 1.f));
		}

		UClass* TileMapClass = LoadClass<ATileMap>(nullptr,
			TEXT("/Game/BP/TileMap/BP_TileMap.BP_TileMap_C"));
		ATileMap* TileMap = Cast<ATileMap>(Remember(World.SpawnActor<ATileMap>(
			TileMapClass != nullptr ? TileMapClass : ATileMap::StaticClass(),
			FVector(0.f, 0.f, 25.f), FRotator::ZeroRotator, Params)));
		if (TileMap != nullptr)
		{
			UTileMapModel* Model = NewObject<UTileMapModel>(TileMap,
				TEXT("SkillPreviewTileMapModel"), RF_Transient);
			Model->SetDimensions(8, 6);
			TileMap->BindModel(Model);
			const TArray<FTileIndex> AimTiles = {
				FTileIndex(2, 0), FTileIndex(2, 1), FTileIndex(0, 2),
				FTileIndex(1, 2), FTileIndex(2, 2), FTileIndex(3, 2),
				FTileIndex(4, 2), FTileIndex(5, 2), FTileIndex(2, 3),
				FTileIndex(2, 4) };
			const TArray<FTileIndex> SelectTiles = { FTileIndex(5, 2) };
			const TArray<FTileIndex> EffectTiles = {
				FTileIndex(5, 2), FTileIndex(4, 2), FTileIndex(6, 2),
				FTileIndex(5, 1), FTileIndex(5, 3) };
			Model->SetTileHighlight(AimTiles, ETileHighlightFlag::Aim);
			Model->SetTileHighlight(SelectTiles, ETileHighlightFlag::Select);
			Model->SetTileHighlight(EffectTiles, ETileHighlightFlag::Effect);
		}

		// 비게임 에디터 월드에서는 BP_TileMap의 Transient ISM이 SceneCapture의
		// Game show flags에서 빠지는 엔진 제약이 있다. 검수 PNG에서 범위 색도
		// 확인할 수 있도록 동일 좌표에 얇은 3D 마커를 둔다. 인게임 구현은 위의
		// 실제 ATileMap 경로만 사용하며 이 코드는 자동 캡처 테스트에만 존재한다.
		UStaticMesh* TilePlane = LoadObject<UStaticMesh>(nullptr,
			TEXT("/Engine/BasicShapes/Plane.Plane"));
		UMaterialInterface* TileBaseMaterial = LoadObject<UMaterialInterface>(nullptr,
			TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		auto SpawnMarker = [&World, &Params, &Remember, TilePlane,
			TileBaseMaterial](const FTileIndex Tile, const FLinearColor Color,
			const float Height)
		{
			AStaticMeshActor* Marker = Cast<AStaticMeshActor>(Remember(
				World.SpawnActor<AStaticMeshActor>(
					FVector(Tile.mX * 250.f, Tile.mY * 250.f, Height),
					FRotator::ZeroRotator, Params)));
			if (Marker == nullptr)
			{
				return;
			}
			Marker->GetStaticMeshComponent()->SetStaticMesh(TilePlane);
			Marker->SetActorScale3D(FVector(2.28f, 2.28f, 1.f));
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
				TileBaseMaterial, Marker);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Marker->GetStaticMeshComponent()->SetMaterial(0, Material);
		};
		for (const FTileIndex Tile : { FTileIndex(2, 0), FTileIndex(2, 1),
			FTileIndex(0, 2), FTileIndex(1, 2), FTileIndex(2, 2),
			FTileIndex(3, 2), FTileIndex(4, 2), FTileIndex(2, 3),
			FTileIndex(2, 4) })
		{
			SpawnMarker(Tile, FLinearColor(.08f, .28f, .48f, 1.f), 22.f);
		}
		for (const FTileIndex Tile : { FTileIndex(4, 2), FTileIndex(5, 2),
			FTileIndex(6, 2), FTileIndex(5, 1), FTileIndex(5, 3) })
		{
			SpawnMarker(Tile, FLinearColor(.64f, .055f, .025f, 1.f), 24.f);
		}
		SpawnMarker(FTileIndex(5, 2), FLinearColor(1.f, .58f, .03f, 1.f), 26.f);

		auto SpawnCharacter = [&World, &Params, &Remember](const TCHAR* MeshPath,
			const FTileIndex Tile, const float Yaw, const float Scale)
		{
			USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, MeshPath);
			ASkeletalMeshActor* Actor = Cast<ASkeletalMeshActor>(Remember(
				World.SpawnActor<ASkeletalMeshActor>(
					FVector(Tile.mX * 250.f, Tile.mY * 250.f, 30.f),
					FRotator(0.f, Yaw, 0.f), Params)));
			if (Actor != nullptr)
			{
				Actor->GetSkeletalMeshComponent()->SetSkeletalMeshAsset(Mesh);
				Actor->SetActorScale3D(FVector(Scale));
			}
		};
		SpawnCharacter(TEXT("/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/"
			"Charaters/Knight/SkeletalMeshes/SK_Knight.SK_Knight"),
			FTileIndex(2, 2), 45.f, 1.45f);
		SpawnCharacter(TEXT("/Game/SVN/OutSideAsset/Kenney/KatKit_Characture/"
			"Charaters/Mage/SkeletalMeshes/SK_Mage.SK_Mage"),
			FTileIndex(1, 4), 35.f, 1.35f);
		SpawnCharacter(TEXT("/Game/SVN/OutSideAsset/Kenney/KatKit_Skeletons/"
			"Characters/Skeleton_Warrior/SkeletalMeshes/"
			"SK_Skeleton_Warrior.SK_Skeleton_Warrior"),
			FTileIndex(5, 2), -135.f, 1.45f);
		SpawnCharacter(TEXT("/Game/SVN/OutSideAsset/Kenney/KatKit_Skeletons/"
			"Characters/Skeleton_Golem/SkeletalMeshes/"
			"SK_Skeleton_Golem.SK_Skeleton_Golem"),
			FTileIndex(6, 4), -135.f, 1.25f);

		ADirectionalLight* Sun = Cast<ADirectionalLight>(Remember(
			World.SpawnActor<ADirectionalLight>(FVector::ZeroVector,
				FRotator(-58.f, -35.f, 0.f), Params)));
		if (Sun != nullptr)
		{
			Sun->GetLightComponent()->SetIntensity(5.f);
			Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f, .82f, .63f));
		}
		APointLight* Fill = Cast<APointLight>(Remember(World.SpawnActor<APointLight>(
			FVector(900.f, 600.f, 1300.f), FRotator::ZeroRotator, Params)));
		if (Fill != nullptr)
		{
			Fill->PointLightComponent->SetIntensity(18000.f);
			Fill->PointLightComponent->SetAttenuationRadius(3500.f);
			Fill->PointLightComponent->SetLightColor(FLinearColor(.46f, .66f, 1.f));
		}

		ACameraActor* Camera = Cast<ACameraActor>(Remember(
			World.SpawnActor<ACameraActor>(FVector::ZeroVector,
				FRotator::ZeroRotator, Params)));
		if (Camera != nullptr)
		{
			const FVector Focus(875.f, 625.f, 30.f);
			const FVector CameraLocation(-1050.f, -1250.f, 2250.f);
			Camera->SetActorLocation(CameraLocation);
			Camera->SetActorRotation((Focus - CameraLocation).Rotation());
			Camera->GetCameraComponent()->ProjectionMode =
				ECameraProjectionMode::Orthographic;
			Camera->GetCameraComponent()->OrthoWidth = 2250.f;
		}
		return Scene;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDetailCaptureTest,
	"P_RD.UI.CombatDetails.CaptureAll",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDetailCaptureTest::RunTest(const FString& Parameters)
{
	using namespace CombatDetailCaptureTests;
	if (GUsingNullRHI)
	{
		AddInfo(TEXT("NullRHI에서는 상세 렌더 캡처를 건너뜀"));
		return true;
	}
	UWorld* World = GEditor != nullptr
		? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!TestNotNull(TEXT("상세 캡처 에디터 월드"), World))
	{
		return false;
	}

	FString Error;
	{
		FDetailFixture Fixture = MakeFixture(*World);
		if (!TestNotNull(TEXT("용병 상세 HUD"), Fixture.HUD.Get()))
		{
			return false;
		}
		UButton* Menu = Cast<UButton>(Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MenuButton_1")));
		UButton* Party = Cast<UButton>(Fixture.HUD->WidgetTree->FindWidget(
			TEXT("PartyButton_0")));
		if (!TestNotNull(TEXT("용병 메뉴 버튼"), Menu)
			|| !TestNotNull(TEXT("첫 용병 버튼"), Party))
		{
			return false;
		}
		Menu->OnClicked.Broadcast();
		Party->OnClicked.Broadcast();
		Fixture.Model->SetUnitDetail(Fixture.KnightDetail);
		UWidget* Roster = Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MercRosterSection"));
		UCanvasPanelSlot* RosterSlot = Roster != nullptr
			? Cast<UCanvasPanelSlot>(Roster->Slot) : nullptr;
		if (TestNotNull(TEXT("용병 목록 공용 열 슬롯"), RosterSlot))
		{
			TestEqual(TEXT("용병 목록 X가 몬스터 목록 열과 일치"),
				RosterSlot->GetPosition().X, 286.0);
			TestEqual(TEXT("용병 목록 Y가 몬스터 목록 열과 일치"),
				RosterSlot->GetPosition().Y, 270.0);
		}
		UWidget* DetailSection = Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MercDetailSection"));
		UCanvasPanelSlot* DetailSectionSlot = DetailSection != nullptr
			? Cast<UCanvasPanelSlot>(DetailSection->Slot) : nullptr;
		if (TestNotNull(TEXT("용병 오른쪽 상세 묶음 슬롯"), DetailSectionSlot))
		{
			TestEqual(TEXT("용병 오른쪽 상세를 한 단계 더 오른쪽으로 배치"),
				DetailSectionSlot->GetPosition().X, 48.0);
		}
		UTextBlock* Name = Cast<UTextBlock>(Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MercenaryDetailName")));
		if (TestNotNull(TEXT("용병 상세 이름"), Name))
		{
			TestEqual(TEXT("용병 상세 데이터 반영"), Name->GetText().ToString(),
				Fixture.KnightDetail.mName.ToString());
		}
		Error.Reset();
		if (!Capture(*Fixture.HUD, nullptr,
			TEXT("CombatDetail_Mercenary.png"), Error))
		{
			AddError(Error);
			return false;
		}

	}

	{
		FDetailFixture Fixture = MakeFixture(*World);
		if (!TestNotNull(TEXT("인벤토리 캡처 HUD"), Fixture.HUD.Get()))
		{
			return false;
		}
		UButton* Menu = Cast<UButton>(Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MenuButton_1")));
		if (!TestNotNull(TEXT("인벤토리 진입용 용병 메뉴 버튼"), Menu))
		{
			return false;
		}
		Menu->OnClicked.Broadcast();
		Fixture.HUD->ShowMercenaryInventoryForTest();
		UWidget* InventoryPage = Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MercenaryInventoryPage"));
		if (TestNotNull(TEXT("용병 인벤토리 페이지"), InventoryPage))
		{
			TestTrue(TEXT("인벤토리 페이지가 열림"),
				InventoryPage->GetVisibility() != ESlateVisibility::Collapsed);
			if (UCanvasPanelSlot* InventorySlot =
				Cast<UCanvasPanelSlot>(InventoryPage->Slot))
			{
				TestEqual(TEXT("인벤토리 오른쪽 묶음을 왼쪽으로 조정"),
					InventorySlot->GetPosition().X, 560.0);
				TestEqual(TEXT("인벤토리 오른쪽 묶음을 위로 조정"),
					InventorySlot->GetPosition().Y, 194.0);
			}
		}
		Error.Reset();
		if (!Capture(*Fixture.HUD, nullptr,
			TEXT("CombatDetail_Inventory.png"), Error))
		{
			AddError(Error);
			return false;
		}
	}

	{
		FDetailFixture Fixture = MakeFixture(*World);
		if (!TestNotNull(TEXT("몬스터 상세 HUD"), Fixture.HUD.Get()))
		{
			return false;
		}
		FCombatTargetUI Target;
		Target.mIsValid = true;
		Target.mUnitId = Fixture.Spider.mUnitId;
		Fixture.Model->SetTarget(Target);
		UButton* Menu = Cast<UButton>(Fixture.HUD->WidgetTree->FindWidget(
			TEXT("MenuButton_2")));
		if (!TestNotNull(TEXT("몬스터 메뉴 버튼"), Menu))
		{
			return false;
		}
		Menu->OnClicked.Broadcast();
		Fixture.Model->SetUnitDetail(Fixture.SpiderDetail);
		UUserWidget* MonsterTab = Fixture.HUD->GetMonsterTabWidgetForTest();
		if (!TestNotNull(TEXT("실제 몬스터 상세 탭"), MonsterTab))
		{
			return false;
		}
		UTextBlock* Name = Cast<UTextBlock>(MonsterTab->GetWidgetFromName(
			TEXT("MonsterDetailNameText")));
		if (TestNotNull(TEXT("몬스터 상세 이름"), Name))
		{
			TestEqual(TEXT("몬스터 상세 데이터 반영"), Name->GetText().ToString(),
				Fixture.SpiderDetail.mName.ToString());
		}
		Error.Reset();
		if (!Capture(*Fixture.HUD, MonsterTab,
			TEXT("CombatDetail_Monster.png"), Error))
		{
			AddError(Error);
			return false;
		}
	}

	{
		FWorldPreviewScene PreviewScene = BuildWorldPreviewScene(*World);
		FDetailFixture Fixture = MakeFixture(*World);
		if (!TestNotNull(TEXT("스킬 상세 HUD"), Fixture.HUD.Get()))
		{
			return false;
		}
		Fixture.Model->SetSkillDetail(Fixture.SkillDetail);
		UUserWidget* Overlay = Fixture.HUD->GetDetailOverlayWidgetForTest();
		if (!TestNotNull(TEXT("실제 스킬 상세 WBP"), Overlay))
		{
			return false;
		}
		UTextBlock* Title = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailTitleText")));
		if (TestNotNull(TEXT("스킬 상세 제목"), Title))
		{
			TestEqual(TEXT("스킬 상세 데이터 반영"), Title->GetText().ToString(),
				Fixture.SkillDetail.mName.ToString());
		}
		if (UTextBlock* Subtitle = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailSubtitleText"))))
		{
			TestEqual(TEXT("기존 텍스트 수치줄은 시각 메달로 대체"),
				Subtitle->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* Preview = Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillVisualPreview")))
		{
			TestEqual(TEXT("통합 스킬 상세 레이아웃 표시"), Preview->GetVisibility(),
				ESlateVisibility::SelfHitTestInvisible);
		}
		UWidget* DescriptionScroll = Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillDescriptionScroll"));
		if (TestNotNull(TEXT("긴 설명용 스크롤 영역"), DescriptionScroll))
		{
			TestEqual(TEXT("기본 상태에서 우측 설명 표시"),
				DescriptionScroll->GetVisibility(),
				ESlateVisibility::Visible);
		}
		UButton* RuntimeSelectButton = Cast<UButton>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillSelectRangeButton")));
		UButton* RuntimeEffectButton = Cast<UButton>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillEffectRangeButton")));
		if (TestNotNull(TEXT("아이콘 아래 사정 범위 버튼"), RuntimeSelectButton))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(
				RuntimeSelectButton->Slot))
			{
				TestEqual(TEXT("사정 범위 버튼 왼쪽 요약열 X"),
					Slot->GetPosition().X, 368.0);
				TestEqual(TEXT("사정 범위 버튼은 수치 아래 Y"),
					Slot->GetPosition().Y, 700.0);
				TestEqual(TEXT("사정 범위 버튼 얇은 정보행 폭"),
					Slot->GetSize().X, 316.0);
				TestEqual(TEXT("사정 범위 버튼 얇은 정보행 높이"),
					Slot->GetSize().Y, 40.0);
			}
		}
		if (TestNotNull(TEXT("아이콘 아래 영향 범위 버튼"), RuntimeEffectButton))
		{
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(
				RuntimeEffectButton->Slot))
			{
				TestEqual(TEXT("영향 범위 버튼도 왼쪽 요약열 기준선 사용"),
					Slot->GetPosition().X, 368.0);
				TestEqual(TEXT("영향 범위 버튼은 사정 범위 아래"),
					Slot->GetPosition().Y, 747.0);
			}
		}
		USkillTacticalDiagramWidget* Tactical =
			Fixture.HUD->GetSkillTacticalDiagramForTest();
		if (TestNotNull(TEXT("전용 스킬 전술 WBP 인스턴스"), Tactical))
		{
			TestEqual(TEXT("전술 WBP 표시"), Tactical->GetVisibility(),
				ESlateVisibility::SelfHitTestInvisible);
			UTextBlock* Select = Cast<UTextBlock>(Tactical->GetWidgetFromName(
				TEXT("TacticalSelectLegendText")));
			if (TestNotNull(TEXT("전술 WBP 선택거리 중앙정렬 라벨"), Select))
			{
				const FString ExpectedRange = FText::AsNumber(
					Fixture.SkillDetail.mTargeting.mSelectRange).ToString();
				TestTrue(TEXT("전술 WBP가 DTO 선택거리 사용"),
					Select->GetText().ToString().Contains(ExpectedRange));
			}
			int32 TacticalCellCount = 0;
			for (int32 Row = 0; Row < 9; ++Row)
			{
				for (int32 Column = 0; Column < 9; ++Column)
				{
					TacticalCellCount += Tactical->GetWidgetFromName(FName(
						*FString::Printf(TEXT("TacticalCell_R%dC%d"), Row,
							Column))) != nullptr ? 1 : 0;
				}
			}
			TestEqual(TEXT("실제 타일형 9x9 전술 셀 81개"),
				TacticalCellCount, 81);
			if (UWidget* Board = Tactical->GetWidgetFromName(
				TEXT("TacticalBoardTilt")))
			{
				TestEqual(TEXT("기본 화면에서 범위판 숨김"),
					Board->GetVisibility(), ESlateVisibility::Collapsed);
			}
			TestNotNull(TEXT("사정 범위 터치 버튼"),
				Tactical->GetWidgetFromName(TEXT("TacticalSelectLegendButton")));
			TestNotNull(TEXT("영향 범위 터치 버튼"),
				Tactical->GetWidgetFromName(TEXT("TacticalEffectLegendButton")));
			if (UWidget* LegendRule = Tactical->GetWidgetFromName(
				TEXT("TacticalLegendRule")))
			{
				TestEqual(TEXT("범위 버튼 사이를 가로지르는 장식선 제거"),
					LegendRule->GetVisibility(), ESlateVisibility::Collapsed);
			}
		}
		if (UWidget* WorldPreview = Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillWorldPreview")))
		{
			TestEqual(TEXT("전투 RenderTarget은 표시하지 않음"),
				WorldPreview->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* OldCell = Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillRangeCell_R0C0")))
		{
			TestEqual(TEXT("실제 전장이 있으면 모식도 셀 제거"),
				OldCell->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UTextBlock* AP = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillStatText_0"))))
		{
			TestEqual(TEXT("AP 텍스트가 실제 DTO를 사용"), AP->GetText().ToString(),
				FString(TEXT("AP 4")));
			if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(AP->Slot))
			{
				TestEqual(TEXT("AP 수치를 왼쪽 정보열에 배치 X"),
					Slot->GetPosition().X, 430.0);
				TestEqual(TEXT("AP 수치를 큰 아이콘 아래에 배치 Y"),
					Slot->GetPosition().Y, 525.0);
			}
		}
		const FName RuntimeStatIconNames[] = {
			TEXT("RuntimeSkillStatIcon_0"), TEXT("RuntimeSkillStatIcon_1"),
			TEXT("RuntimeSkillStatIcon_2"), TEXT("RuntimeSkillStatIcon_3") };
		const FName HudSourceIconNames[] = {
			TEXT("CommandCostBadge_0"), NAME_None,
			TEXT("CommandCooldownBadge_0"), NAME_None };
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(RuntimeStatIconNames); ++Index)
		{
			UImage* RuntimeIcon = Cast<UImage>(Overlay->GetWidgetFromName(
				RuntimeStatIconNames[Index]));
			if (TestNotNull(*FString::Printf(TEXT("전투 HUD 수치 아이콘 %d"), Index),
				RuntimeIcon) && HudSourceIconNames[Index] != NAME_None)
			{
				UImage* SourceIcon = Cast<UImage>(Fixture.HUD->GetWidgetFromName(
					HudSourceIconNames[Index]));
				if (TestNotNull(*FString::Printf(TEXT("전투 HUD 원본 아이콘 %d"), Index),
					SourceIcon))
				{
					TestEqual(*FString::Printf(TEXT("WBP 실제 브러시 재사용 %d"), Index),
						RuntimeIcon->GetBrush().GetResourceObject(),
						SourceIcon->GetBrush().GetResourceObject());
				}
			}
		}
		if (UImage* DamageIcon = Cast<UImage>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillStatIcon_1"))))
		{
			UTexture2D* ExpectedDamage = LoadTexture(TEXT(
				"/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
				"T_SkillStat_Damage_Simple_v2.T_SkillStat_Damage_Simple_v2"));
			if (TestNotNull(TEXT("모바일용 단순 피해 에셋"), ExpectedDamage))
			{
				TestEqual(TEXT("피해 행이 단순 전용 에셋 사용"),
					DamageIcon->GetBrush().GetResourceObject(),
					static_cast<UObject*>(ExpectedDamage));
			}
		}
		if (UImage* CriticalIcon = Cast<UImage>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillStatIcon_3"))))
		{
			UTexture2D* ExpectedCritical = LoadTexture(TEXT(
				"/Game/SVN/OutSideAsset/AICreation/UI/CombatDetail/SkillTactical/"
				"T_SkillStat_Critical_Simple_v2.T_SkillStat_Critical_Simple_v2"));
			if (TestNotNull(TEXT("모바일용 단순 치명타 에셋"), ExpectedCritical))
			{
				TestEqual(TEXT("치명타 행이 단순 전용 에셋 사용"),
					CriticalIcon->GetBrush().GetResourceObject(),
					static_cast<UObject*>(ExpectedCritical));
			}
		}
		if (UWidget* RuntimeScale = Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillDesignScale")))
		{
			TestTrue(TEXT("런타임 상세가 1920 디자인 ScaleBox를 사용"),
				RuntimeScale->IsA<UScaleBox>());
		}
		if (UTextBlock* SelectLegend = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("RuntimeSkillSelectLegend"))))
		{
			TestEqual(TEXT("구형 합성 범위 라벨 제거"),
				SelectLegend->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* TargetBlock = Overlay->GetWidgetFromName(
			TEXT("DetailTargetBlock")))
		{
			TestEqual(TEXT("별도 사거리 도식 에셋 제거"), TargetBlock->GetVisibility(),
				ESlateVisibility::Collapsed);
		}
		if (UWidget* RightColumn = Overlay->GetWidgetFromName(
			TEXT("DetailRightColumn")))
		{
			TestEqual(TEXT("사거리 전용 오른쪽 판 제거"),
				RightColumn->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* Divider = Overlay->GetWidgetFromName(TEXT("DetailDivider_0")))
		{
			TestEqual(TEXT("스킬 상세의 하단 분할선 제거"),
				Divider->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* FreePlate = Overlay->GetWidgetFromName(TEXT("DetailFreePlate")))
		{
			TestEqual(TEXT("스킬 상세의 별도 사거리 받침판 제거"),
				FreePlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* IdentityPlate = Overlay->GetWidgetFromName(
			TEXT("DetailIdentityPlate")))
		{
			TestEqual(TEXT("스킬 수치 내부 프레임 제거"),
				IdentityPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* StatsPlate = Overlay->GetWidgetFromName(TEXT("DetailStatsPlate")))
		{
			TestEqual(TEXT("스킬 상단 수치 양피지 제거"),
				StatsPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UTextBlock* Body = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailBodyText"))))
		{
			TestTrue(TEXT("스킬 설명 본문 유지"),
				Body->GetText().ToString().Contains(TEXT("주변의 모든 적")));
			TestFalse(TEXT("범위 설명을 본문에서 중복하지 않음"),
				Body->GetText().ToString().Contains(TEXT("대상 선택"))
					|| Body->GetText().ToString().Contains(TEXT("적용 범위")));
		}
		Error.Reset();
		if (!Capture(*Fixture.HUD, Overlay,
			TEXT("CombatDetail_Skill_TacticalWBP.png"), Error))
		{
			AddError(Error);
			PreviewScene.Destroy();
			return false;
		}
		if (Tactical != nullptr && RuntimeSelectButton != nullptr
			&& RuntimeEffectButton != nullptr)
		{
			RuntimeSelectButton->OnClicked.Broadcast();
			if (UWidget* Board = Tactical->GetWidgetFromName(
				TEXT("TacticalBoardTilt")))
			{
				TestEqual(TEXT("사정 범위 터치 후 전술판 표시"),
					Board->GetVisibility(), ESlateVisibility::HitTestInvisible);
			}
			if (DescriptionScroll != nullptr)
			{
				TestEqual(TEXT("범위판 표시 중 설명 스크롤 숨김"),
					DescriptionScroll->GetVisibility(), ESlateVisibility::Collapsed);
			}
			Error.Reset();
			if (!Capture(*Fixture.HUD, Overlay,
				TEXT("CombatDetail_Skill_SelectRange.png"), Error))
			{
				AddError(Error);
				PreviewScene.Destroy();
				return false;
			}

			RuntimeEffectButton->OnClicked.Broadcast();
			Error.Reset();
			if (!Capture(*Fixture.HUD, Overlay,
				TEXT("CombatDetail_Skill_EffectRange.png"), Error))
			{
				AddError(Error);
				PreviewScene.Destroy();
				return false;
			}
			RuntimeEffectButton->OnClicked.Broadcast();
			if (DescriptionScroll != nullptr)
			{
				TestEqual(TEXT("범위 버튼 재터치 후 설명 복귀"),
					DescriptionScroll->GetVisibility(),
					ESlateVisibility::Visible);
			}
		}
		PreviewScene.Destroy();
	}

	{
		FDetailFixture Fixture = MakeFixture(*World);
		if (!TestNotNull(TEXT("아티팩트 상세 HUD"), Fixture.HUD.Get()))
		{
			return false;
		}
		Fixture.HUD->ShowArtifactDetailForTest(0);
		UUserWidget* Overlay = Fixture.HUD->GetDetailOverlayWidgetForTest();
		if (!TestNotNull(TEXT("실제 아티팩트 상세 WBP"), Overlay))
		{
			return false;
		}
		UTextBlock* Title = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailTitleText")));
		if (TestNotNull(TEXT("아티팩트 상세 제목"), Title))
		{
			TestEqual(TEXT("아티팩트 상세 데이터 반영"),
				Title->GetText().ToString(),
				Fixture.Meta.mArtifacts[0].mName.ToString());
		}
		if (UTextBlock* Subtitle = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailSubtitleText"))))
		{
			TestFalse(TEXT("아티팩트 상세에는 판매가를 표시하지 않음"),
				Subtitle->GetText().ToString().Contains(TEXT("판매가")));
		}
		// 프레젠터 계약(PresentArtifact): 공용 본문은 데이터만 유지하고, 실제
		// 표시는 전용 ScrollBox 본문이 맡는다 -- 긴 효과 줄도 잘리지 않는다.
		if (UTextBlock* Body = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("DetailBodyText"))))
		{
			TestTrue(TEXT("아티팩트 효과 데이터가 공용 본문에 유지됨"),
				Body->GetText().ToString().Contains(TEXT("HP를 5 회복")));
		}
		if (UTextBlock* ScrollBody = Cast<UTextBlock>(Overlay->GetWidgetFromName(
			TEXT("RuntimeArtifactDescriptionText"))))
		{
			TestTrue(TEXT("아티팩트 효과 본문이 스크롤 본문에 표시됨"),
				ScrollBody->GetText().ToString().Contains(TEXT("HP를 5 회복")));
		}
		else
		{
			AddError(TEXT("아티팩트 효과 스크롤 본문(RuntimeArtifactDescriptionText) 없음"));
		}
		if (UWidget* Scroll = Overlay->GetWidgetFromName(
			TEXT("RuntimeArtifactDescriptionScroll")))
		{
			TestTrue(TEXT("아티팩트 효과 본문이 실제 ScrollBox에 담김"),
				Scroll->IsA<UScrollBox>());
			TestEqual(TEXT("아티팩트 효과 스크롤 표시"),
				Scroll->GetVisibility(), ESlateVisibility::Visible);
			if (UCanvasPanelSlot* ScrollSlot = Cast<UCanvasPanelSlot>(Scroll->Slot))
			{
				TestEqual(TEXT("아티팩트 효과를 아이콘 오른쪽에 배치"),
					ScrollSlot->GetPosition().X, 780.0);
				TestEqual(TEXT("아티팩트 효과를 상단 정보 흐름에 배치"),
					ScrollSlot->GetPosition().Y, 360.0);
			}
		}
		else
		{
			AddError(TEXT("아티팩트 효과 스크롤(RuntimeArtifactDescriptionScroll) 없음"));
		}
		if (UWidget* Divider = Overlay->GetWidgetFromName(TEXT("DetailDivider_0")))
		{
			TestEqual(TEXT("아티팩트 화면 반분 구분선 제거"),
				Divider->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* IdentityPlate = Overlay->GetWidgetFromName(
			TEXT("DetailIdentityPlate")))
		{
			TestEqual(TEXT("아티팩트 메타 내부 프레임 제거"),
				IdentityPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		if (UWidget* StatsPlate = Overlay->GetWidgetFromName(TEXT("DetailStatsPlate")))
		{
			TestEqual(TEXT("아티팩트 상단 메타 양피지 제거"),
				StatsPlate->GetVisibility(), ESlateVisibility::Collapsed);
		}
		Error.Reset();
		if (!Capture(*Fixture.HUD, Overlay,
			TEXT("CombatDetail_Artifact.png"), Error))
		{
			AddError(Error);
			return false;
		}
	}

	for (const TCHAR* Name : { TEXT("CombatDetail_Mercenary.png"),
		TEXT("CombatDetail_Inventory.png"),
		TEXT("CombatDetail_Monster.png"),
		TEXT("CombatDetail_Skill_TacticalWBP.png"),
		TEXT("CombatDetail_Skill_SelectRange.png"),
		TEXT("CombatDetail_Skill_EffectRange.png"),
		TEXT("CombatDetail_Artifact.png") })
	{
		const FString Path = FPaths::Combine(OutputDirectory(), Name);
		TestTrue(*FString::Printf(TEXT("%s 생성"), Name),
			IFileManager::Get().FileExists(*Path));
		TestTrue(*FString::Printf(TEXT("%s 내용 있음"), Name),
			IFileManager::Get().FileSize(*Path) > 100000);
	}
	return true;
}

#endif
