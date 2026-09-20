#include "UI/RewardConceptBoardBuilder.h"

#include "AssetToolsModule.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "UI/UIFont.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditorUtils.h"
#include "WidgetBlueprintFactory.h"

/**
 * 보상 흐름 시안 2/3/6을 실제 파츠 텍스처로 조립한 비교용 WBP 12장
 * (시안 3종 x 단계 4종). reward-flow-slots.json 의 좌표(RewardConceptSlots.inl,
 * 코드젠)로 배치하고, 아트는 V11/V12 파츠 라이브러리를 재사용한다.
 * 승자 시안이 정해지면 그 시안만 전용 아트·연출을 입힌다.
 */
namespace RewardConceptBoardBuilder
{
	struct FConceptSlot
	{
		const TCHAR* Name;
		float X, Y, W, H;
		int32 Layer;
		uint8 Deco;
		bool bText;
	};

	struct FConceptStage
	{
		const TCHAR* Concept;
		const TCHAR* Stage;
		const FConceptSlot* Slots;
		int32 Num;
	};

#include "UI/RewardConceptSlots.inl"
#include "UI/RewardC03Slots.inl"
#include "UI/RewardBSSlots.inl"

	constexpr TCHAR PackagePath[] = TEXT("/Game/UI/RewardSettlement/Concepts");
	TUniquePtr<FAutoConsoleCommand> BuildCommand;
	TUniquePtr<FAutoConsoleCommand> BuildC03Command;
	TUniquePtr<FAutoConsoleCommand> BuildBSCommand;

	UTexture2D* Texture(const TCHAR* Path)
	{
		UTexture2D* Result = LoadObject<UTexture2D>(nullptr, Path);
		checkf(Result != nullptr, TEXT("Missing concept board texture: %s"), Path);
		return Result;
	}

	struct FTextureSet
	{
		UTexture2D* ModalBackground;
		UTexture2D* ParchmentSheet;
		UTexture2D* HeaderPlate;
		UTexture2D* StepPlate;
		UTexture2D* CtaPlate;
		UTexture2D* ExpRowPlate;
		UTexture2D* ExpPortraitRing;
		UTexture2D* ExpLevelWindow;
		UTexture2D* ExpProgressTrack;
		UTexture2D* ExpXpBadge;
		UTexture2D* ProgressFill;
		UTexture2D* GoldCoinRing;
		UTexture2D* GoldAmountWindow;
		UTexture2D* CardFrame;
		UTexture2D* CardNamePlate;
		UTexture2D* CardSelectedOverlay;
		UTexture2D* ChestClosed;
		UTexture2D* ChestAura;
		UTexture2D* ChestBurst;
		UTexture2D* GoldCoin;
		UTexture2D* Portraits[3];
		UTexture2D* Artifacts[3];
	};

	FTextureSet LoadTextures()
	{
		FTextureSet T;
		T.ModalBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/T_R11_ModalBackground.T_R11_ModalBackground"));
		T.ParchmentSheet = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/T_R11_ParchmentSheet.T_R11_ParchmentSheet"));
		T.HeaderPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/T_R11_HeaderPlate.T_R11_HeaderPlate"));
		T.StepPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/T_R11_StepPlate.T_R11_StepPlate"));
		T.CtaPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV11/T_R11_CtaPlate.T_R11_CtaPlate"));
		T.ExpRowPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_ExpRowPlate.T_R12_ExpRowPlate"));
		T.ExpPortraitRing = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_ExpPortraitRing.T_R12_ExpPortraitRing"));
		T.ExpLevelWindow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_ExpLevelWindow.T_R12_ExpLevelWindow"));
		T.ExpProgressTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_ExpProgressTrack.T_R12_ExpProgressTrack"));
		T.ExpXpBadge = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_ExpXpBadge.T_R12_ExpXpBadge"));
		T.ProgressFill = Texture(TEXT("/Game/UI/ResultBoards/Art/T_VR_ProgressFill_0809.T_VR_ProgressFill_0809"));
		T.GoldCoinRing = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_GoldCoinRing.T_R12_GoldCoinRing"));
		T.GoldAmountWindow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_GoldAmountWindow.T_R12_GoldAmountWindow"));
		T.CardFrame = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_CardFrame.T_R12_CardFrame"));
		T.CardNamePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_CardNamePlate.T_R12_CardNamePlate"));
		T.CardSelectedOverlay = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/AtomicV12/T_R12_CardSelectedOverlay.T_R12_CardSelectedOverlay"));
		T.ChestClosed = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestClosed.T_RS_V8_ChestClosed"));
		T.ChestAura = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestAura.T_RS_V8_ChestAura"));
		T.ChestBurst = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/V8PartsV2/T_RS_V8_ChestBurst.T_RS_V8_ChestBurst"));
		T.GoldCoin = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Reward/T_Reward_GoldIcon_V1.T_Reward_GoldIcon_V1"));
		T.Portraits[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		T.Portraits[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		T.Portraits[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		T.Artifacts[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice"));
		T.Artifacts[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet"));
		T.Artifacts[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"));
		return T;
	}

	FSlateBrush TextureBrush(UTexture2D* Source)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Source);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	void Place(UCanvasPanel* Parent, UWidget* Child, const FVector2D Position,
		const FVector2D Size, int32 ZOrder)
	{
		UCanvasPanelSlot* CanvasSlot = Parent->AddChildToCanvas(Child);
		CanvasSlot->SetAnchors(FAnchors(0.f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetSize(Size);
		CanvasSlot->SetZOrder(ZOrder);
	}

	UImage* AddImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FString& Name,
		UTexture2D* Source, const FVector2D Position, const FVector2D Size, int32 ZOrder)
	{
		UImage* Image = Blueprint->WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), FName(*Name));
		Image->SetBrush(TextureBrush(Source));
		Image->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Image, Position, Size, ZOrder);
		return Image;
	}

	FVector2D AspectFitSize(UTexture2D* Source, const FVector2D Bounds)
	{
		const FIntPoint TextureSize(Source->GetSizeX(), Source->GetSizeY());
		if (TextureSize.X <= 0 || TextureSize.Y <= 0)
		{
			return Bounds;
		}
		const FVector2D NativeSize(TextureSize);
		const double Scale = FMath::Min(Bounds.X / NativeSize.X, Bounds.Y / NativeSize.Y);
		return NativeSize * Scale;
	}

	UImage* AddAspectImage(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FString& Name,
		UTexture2D* Source, const FVector2D BoundsPosition, const FVector2D BoundsSize, int32 ZOrder)
	{
		const FVector2D Fitted = AspectFitSize(Source, BoundsSize);
		return AddImage(Blueprint, Parent, Name, Source,
			BoundsPosition + (BoundsSize - Fitted) * 0.5f, Fitted, ZOrder);
	}

	UTextBlock* AddText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FString& Name,
		const FString& Value, int32 Size, const FVector2D Position, const FVector2D Box,
		int32 ZOrder, ETextJustify::Type Justify = ETextJustify::Center)
	{
		UTextBlock* Text = Blueprint->WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), FName(*Name));
		Text->SetText(FText::FromString(Value));
		FSlateFontInfo Font = UIFont::MakeProjectExact(Text->GetFont(), Size);
		Font.OutlineSettings.OutlineSize = Size >= 28 ? 1 : 0;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Text->SetShadowOffset(FVector2D(1.5f, 1.5f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
		Text->SetJustification(Justify);
		Text->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		Place(Parent, Text, Position, Box, ZOrder);
		return Text;
	}

	/** 양피지 위 짙은 갈색 텍스트 (목업 표기 관례). */
	UTextBlock* AddInkText(UWidgetBlueprint* Blueprint, UCanvasPanel* Parent, const FString& Name,
		const FString& Value, int32 Size, const FVector2D Position, const FVector2D Box,
		int32 ZOrder, ETextJustify::Type Justify = ETextJustify::Center)
	{
		UTextBlock* Text = AddText(Blueprint, Parent, Name, Value, Size, Position, Box, ZOrder, Justify);
		FSlateFontInfo Font = Text->GetFont();
		Font.OutlineSettings.OutlineSize = 0;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.16f, 0.10f, 0.05f)));
		Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
		return Text;
	}

	FString BaseSlotName(const FString& Name, int32& OutIndex)
	{
		OutIndex = 0;
		int32 UnderscoreIndex = INDEX_NONE;
		if (Name.FindLastChar(TEXT('_'), UnderscoreIndex) && UnderscoreIndex < Name.Len() - 1)
		{
			const FString Tail = Name.Mid(UnderscoreIndex + 1);
			if (Tail.IsNumeric())
			{
				OutIndex = FCString::Atoi(*Tail) - 1;
				return Name.Left(UnderscoreIndex);
			}
		}
		return Name;
	}

	int32 StageIndex(const FString& Stage)
	{
		if (Stage == TEXT("experience")) { return 0; }
		if (Stage == TEXT("chest")) { return 1; }
		if (Stage == TEXT("gold")) { return 2; }
		return 3;
	}

	FString StageKorean(int32 Index)
	{
		switch (Index)
		{
		case 0: return TEXT("경험치");
		case 1: return TEXT("상자");
		case 2: return TEXT("골드");
		default: return TEXT("아티팩트");
		}
	}

	FString ProgressLine(int32 Current)
	{
		// 프로젝트 폰트에 ●○▶✓→ 글리프가 없어 네모로 깨진다. ASCII만 쓴다.
		FString Line;
		for (int32 Index = 0; Index < 4; ++Index)
		{
			if (!Line.IsEmpty())
			{
				Line += TEXT("  -  ");
			}
			Line += Index == Current
				? FString::Printf(TEXT("[ %s ]"), *StageKorean(Index))
				: StageKorean(Index);
		}
		return Line;
	}

	/** V12 EXP 행 조립을 슬롯 크기에 비례해 재현한다 (기준 1054x132). */
	void BuildMercenaryRow(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FTextureSet& T, const FConceptSlot& S, int32 RowIndex, int32 BaseZ)
	{
		const FString Prefix = FString::Printf(TEXT("row%d"), RowIndex);
		const float SX = S.W / 1054.f;
		const FVector2D Origin(S.X, S.Y);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_plate"), T.ExpRowPlate,
			Origin, FVector2D(S.W, S.H), BaseZ);
		const float RingSize = S.H * 0.88f;
		const FVector2D RingPos = Origin + FVector2D(36.f * SX, (S.H - RingSize) * 0.5f);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_ring"), T.ExpPortraitRing,
			RingPos, FVector2D(RingSize, RingSize), BaseZ + 2);
		AddAspectImage(Blueprint, Canvas, Prefix + TEXT("_portrait"), T.Portraits[RowIndex % 3],
			RingPos + FVector2D(RingSize * 0.1f, RingSize * 0.1f),
			FVector2D(RingSize * 0.8f, RingSize * 0.8f), BaseZ + 3);
		const FVector2D LevelSize(96.f * SX, S.H * 0.44f);
		const FVector2D LevelPos = Origin + FVector2D(196.f * SX, (S.H - LevelSize.Y) * 0.5f);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_level"), T.ExpLevelWindow,
			LevelPos, LevelSize, BaseZ + 2);
		AddText(Blueprint, Canvas, Prefix + TEXT("_leveltext"),
			FString::Printf(TEXT("Lv.%d"), RowIndex + 1), 24,
			LevelPos + FVector2D(0.f, LevelSize.Y * 0.14f),
			FVector2D(LevelSize.X, LevelSize.Y * 0.72f), BaseZ + 3);
		const FVector2D TrackSize(516.f * SX, FMath::Min(22.f, S.H * 0.18f));
		const FVector2D TrackPos = Origin + FVector2D(320.f * SX, S.H * 0.58f);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_track"), T.ExpProgressTrack,
			TrackPos, TrackSize, BaseZ + 2);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_fill"), T.ProgressFill,
			TrackPos + FVector2D(2.f, 2.f),
			FVector2D((TrackSize.X - 4.f) * (0.3f + 0.16f * RowIndex), TrackSize.Y - 4.f), BaseZ + 3);
		AddText(Blueprint, Canvas, Prefix + TEXT("_progresstext"),
			FString::Printf(TEXT("%d / 250"), 75 + 40 * RowIndex), 18,
			TrackPos + FVector2D(0.f, -S.H * 0.3f), FVector2D(TrackSize.X, S.H * 0.28f), BaseZ + 3);
		const FVector2D BadgeSize(120.f * SX, S.H * 0.5f);
		const FVector2D BadgePos = Origin + FVector2D(886.f * SX, (S.H - BadgeSize.Y) * 0.5f);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_badge"), T.ExpXpBadge,
			BadgePos, BadgeSize, BaseZ + 2);
		AddText(Blueprint, Canvas, Prefix + TEXT("_badgetext"), TEXT("+50 XP"), 20,
			BadgePos + FVector2D(0.f, BadgeSize.Y * 0.18f),
			FVector2D(BadgeSize.X, BadgeSize.Y * 0.64f), BaseZ + 3);
	}

	/** 세로형 용병 카드/타일 (시안 2·6). */
	void BuildMercenaryCard(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FTextureSet& T, const FConceptSlot& S, int32 CardIndex, int32 BaseZ)
	{
		static const TCHAR* Names[] = { TEXT("나이트"), TEXT("메이지"), TEXT("로그") };
		const FString Prefix = FString::Printf(TEXT("merc%d"), CardIndex);
		const FVector2D Origin(S.X, S.Y);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_frame"), T.CardFrame,
			Origin, FVector2D(S.W, S.H), BaseZ + 1);
		AddAspectImage(Blueprint, Canvas, Prefix + TEXT("_portrait"), T.Portraits[CardIndex % 3],
			Origin + FVector2D(S.W * 0.2f, S.H * 0.08f),
			FVector2D(S.W * 0.6f, S.H * 0.5f), BaseZ);
		AddText(Blueprint, Canvas, Prefix + TEXT("_name"),
			FString::Printf(TEXT("%s Lv.%d"), Names[CardIndex % 3], CardIndex + 1), 22,
			Origin + FVector2D(S.W * 0.1f, S.H * 0.6f),
			FVector2D(S.W * 0.8f, S.H * 0.16f), BaseZ + 2);
		AddText(Blueprint, Canvas, Prefix + TEXT("_xp"), TEXT("+50 XP"), 20,
			Origin + FVector2D(S.W * 0.1f, S.H * 0.78f),
			FVector2D(S.W * 0.8f, S.H * 0.16f), BaseZ + 2);
	}

	/** 아티팩트 카드. 첫 번째 카드는 선택 상태를 시연한다. */
	void BuildArtifactCard(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FTextureSet& T, const FConceptSlot& S, int32 CardIndex, int32 BaseZ)
	{
		static const TCHAR* Names[] = { TEXT("피의 성배"), TEXT("야수의 송곳니"), TEXT("행운의 주화") };
		const FString Prefix = FString::Printf(TEXT("artifact%d"), CardIndex);
		const FVector2D Origin(S.X, S.Y);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_frame"), T.CardFrame,
			Origin, FVector2D(S.W, S.H * 0.78f), BaseZ + 1);
		AddAspectImage(Blueprint, Canvas, Prefix + TEXT("_icon"), T.Artifacts[CardIndex % 3],
			Origin + FVector2D(S.W * 0.2f, S.H * 0.1f),
			FVector2D(S.W * 0.6f, S.H * 0.5f), BaseZ);
		AddImage(Blueprint, Canvas, Prefix + TEXT("_nameplate"), T.CardNamePlate,
			Origin + FVector2D(0.f, S.H * 0.78f), FVector2D(S.W, S.H * 0.22f), BaseZ + 1);
		AddText(Blueprint, Canvas, Prefix + TEXT("_name"), Names[CardIndex % 3], 20,
			Origin + FVector2D(S.W * 0.08f, S.H * 0.8f),
			FVector2D(S.W * 0.84f, S.H * 0.17f), BaseZ + 2);
		if (CardIndex == 0)
		{
			AddImage(Blueprint, Canvas, Prefix + TEXT("_selected"), T.CardSelectedOverlay,
				Origin, FVector2D(S.W, S.H), BaseZ + 3);
		}
	}

	void BuildSlot(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas, const FTextureSet& T,
		const FConceptSlot& S, int32 CurrentStage)
	{
		int32 PartIndex = 0;
		const FString Base = BaseSlotName(S.Name, PartIndex);
		const FVector2D Origin(S.X, S.Y);
		const FVector2D Size(S.W, S.H);
		const int32 Z = S.Layer * 10;
		const FString Unique = FString(S.Name);

		if (Base == TEXT("upper_scene_background"))
		{
			UBorder* Scene = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), FName(*(Unique + TEXT("_border"))));
			Scene->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.035f, 0.95f));
			Scene->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Canvas, Scene, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_label"), TEXT("( 전투 화면 )"), 18,
				Origin, Size, Z + 1);
		}
		else if (Base == TEXT("sheet_outer_anchor") || Base == TEXT("table_outer_nameplate"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.HeaderPlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_title"), TEXT("전투 보상"), 34,
				Origin + FVector2D(0.f, Size.Y * 0.16f), FVector2D(Size.X, Size.Y * 0.68f), Z + 1);
		}
		else if (Base == TEXT("left_outer_rail_cap"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.HeaderPlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_title"), TEXT("보상"), 28,
				Origin + FVector2D(0.f, Size.Y * 0.16f), FVector2D(Size.X, Size.Y * 0.68f), Z + 1);
		}
		else if (Base == TEXT("sheet_progress_bar") || Base == TEXT("table_step_badges"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.StepPlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), ProgressLine(CurrentStage), 20,
				Origin + FVector2D(0.f, Size.Y * 0.14f), FVector2D(Size.X, Size.Y * 0.72f), Z + 1);
		}
		else if (Base == TEXT("left_stage_checklist"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CardNamePlate, Origin, Size, Z);
			FString Checklist;
			for (int32 Index = 0; Index < 4; ++Index)
			{
				Checklist += FString::Printf(TEXT("%s %s\n\n"),
					Index == CurrentStage ? TEXT(">") : (Index < CurrentStage ? TEXT("*") : TEXT("-")),
					*StageKorean(Index));
			}
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), Checklist, 22,
				Origin + FVector2D(Size.X * 0.1f, Size.Y * 0.1f),
				FVector2D(Size.X * 0.8f, Size.Y * 0.8f), Z + 1, ETextJustify::Left);
		}
		else if (Base == TEXT("content_title_strip"))
		{
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				FString::Printf(TEXT("전투 보상 — %s"), *StageKorean(CurrentStage)), 32,
				Origin + FVector2D(12.f, Size.Y * 0.2f),
				FVector2D(Size.X - 24.f, Size.Y * 0.6f), Z + 1, ETextJustify::Left);
		}
		else if (Base == TEXT("table_legend_background"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CardNamePlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				TEXT("전리품 테이블 — 획득한 보상이 이곳에 정리됩니다"), 18,
				Origin, Size, Z + 1);
		}
		else if (Base == TEXT("mercenary_row"))
		{
			BuildMercenaryRow(Blueprint, Canvas, T, S, PartIndex, Z);
		}
		else if (Base == TEXT("mercenary_card") || Base == TEXT("mercenary_tile"))
		{
			BuildMercenaryCard(Blueprint, Canvas, T, S, PartIndex, Z);
		}
		else if (Base == TEXT("level_up_ticker") || Base == TEXT("level_up_callout")
			|| Base == TEXT("level_up_table_marker"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.GoldAmountWindow, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				TEXT("나이트 Lv.5 -> Lv.6  레벨업!"), 24, Origin, Size, Z + 1);
		}
		else if (Base == TEXT("chest_aura_reserve"))
		{
			UImage* Aura = AddAspectImage(Blueprint, Canvas, Unique + TEXT("_aura"),
				T.ChestAura, Origin, Size, Z);
			Aura->SetRenderOpacity(0.35f);
		}
		else if (Base == TEXT("chest_burst_reserve"))
		{
			UImage* Burst = AddAspectImage(Blueprint, Canvas, Unique + TEXT("_burst"),
				T.ChestBurst, Origin, Size, Z);
			Burst->SetRenderOpacity(0.3f);
		}
		else if (Base == TEXT("chest_touch_visual"))
		{
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_chest"), T.ChestClosed, Origin, Size, Z);
		}
		else if (Base == TEXT("chest_touch_button") || Base == TEXT("gold_terminal_button")
			|| Base == TEXT("artifact_confirm_button"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CtaPlate, Origin, Size, Z);
			const TCHAR* Label = Base == TEXT("chest_touch_button")
				? TEXT("상자 열기")
				: (Base == TEXT("gold_terminal_button") ? TEXT("받기") : TEXT("확정"));
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), Label, 26,
				Origin + FVector2D(0.f, Size.Y * 0.16f), FVector2D(Size.X, Size.Y * 0.68f), Z + 1);
		}
		else if (Base == TEXT("gold_coin_icon_slot"))
		{
			const FVector2D RingBounds(Size.X, Size.Y * 0.72f);
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_ring"), T.GoldCoinRing,
				Origin, RingBounds, Z);
			const FVector2D CoinBounds = RingBounds * 0.62f;
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_coin"), T.GoldCoin,
				Origin + (RingBounds - CoinBounds) * 0.5f, CoinBounds, Z + 1);
			AddText(Blueprint, Canvas, Unique + TEXT("_label"), TEXT("획득 골드"), 24,
				Origin + FVector2D(0.f, Size.Y * 0.76f), FVector2D(Size.X, Size.Y * 0.22f), Z + 1);
		}
		else if (Base == TEXT("gold_amount_window"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.GoldAmountWindow, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("+350 G"), 42,
				Origin + FVector2D(0.f, Size.Y * 0.2f), FVector2D(Size.X, Size.Y * 0.6f), Z + 1);
		}
		else if (Base == TEXT("gold_only_terminal_note"))
		{
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				TEXT("아티팩트 없음 · 골드가 최종 보상입니다"), 18, Origin, Size, Z + 1);
		}
		else if (Base == TEXT("artifact_card"))
		{
			BuildArtifactCard(Blueprint, Canvas, T, S, PartIndex, Z);
		}
		else if (Base == TEXT("selection_state"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CardNamePlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				PartIndex == 0 ? TEXT("선택됨") : TEXT("탭하여 선택"), 20,
				Origin + FVector2D(0.f, Size.Y * 0.14f), FVector2D(Size.X, Size.Y * 0.72f), Z + 1);
		}
		else
		{
			// 새 슬롯 이름이 JSON에 추가되면 조용히 빠뜨리지 않고 빌드를 멈춘다.
			checkf(false, TEXT("Unknown concept slot name: %s"), S.Name);
		}
	}

	void PaintConceptBase(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas,
		const FTextureSet& T, const FString& Concept)
	{
		if (Concept.StartsWith(TEXT("02")))
		{
			AddImage(Blueprint, Canvas, TEXT("base_sheet"), T.ParchmentSheet,
				FVector2D(100.f, 150.f), FVector2D(1336.f, 706.f), 0);
		}
		else if (Concept.StartsWith(TEXT("03")))
		{
			AddImage(Blueprint, Canvas, TEXT("base_modal"), T.ModalBackground,
				FVector2D::ZeroVector, FVector2D(1536.f, 864.f), 0);
			AddImage(Blueprint, Canvas, TEXT("base_content"), T.ParchmentSheet,
				FVector2D(340.f, 130.f), FVector2D(1156.f, 660.f), 0);
		}
		else
		{
			AddImage(Blueprint, Canvas, TEXT("base_table"), T.ParchmentSheet,
				FVector2D::ZeroVector, FVector2D(1536.f, 864.f), 0);
		}
	}

	void BuildStage(const FTextureSet& T, const FConceptStage& Stage)
	{
		const FString ConceptId = FString(Stage.Concept).Left(2);
		FString StageName = FString(Stage.Stage);
		StageName[0] = FChar::ToUpper(StageName[0]);
		const FString AssetName = FString::Printf(TEXT("WBP_RC%s_%s"), *ConceptId, *StageName);
		const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), PackagePath, *AssetName, *AssetName);

		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
		if (Blueprint == nullptr)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = UUserWidget::StaticClass();
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
				AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
		}
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Could not create concept board %s"), *AssetName);
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UUserWidget::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("ConceptRoot"));
		Blueprint->WidgetTree->RootWidget = Root;
		UBorder* Backdrop = Blueprint->WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ConceptBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.008f, 0.009f, 0.011f, 1.f));
		Root->AddChildToOverlay(Backdrop);
		CastChecked<UOverlaySlot>(Backdrop->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Backdrop->Slot)->SetVerticalAlignment(VAlign_Fill);
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("ConceptScale"));
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Root->AddChildToOverlay(Scale);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetVerticalAlignment(VAlign_Fill);
		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ConceptDesignSize"));
		DesignSize->SetWidthOverride(1536.f);
		DesignSize->SetHeightOverride(864.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("ConceptCanvas"));
		DesignSize->SetContent(Canvas);

		PaintConceptBase(Blueprint, Canvas, T, Stage.Concept);
		const int32 Current = StageIndex(Stage.Stage);
		for (int32 Index = 0; Index < Stage.Num; ++Index)
		{
			BuildSlot(Blueprint, Canvas, T, Stage.Slots[Index], Current);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()), TEXT("Could not save concept board %s"), *AssetName);
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_CONCEPT_BUILD saved %s"), *AssetName);
	}

	void Build()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_CONCEPT_BUILD begin"));
		const FTextureSet T = LoadTextures();
		for (const FConceptStage& Stage : GConceptStages)
		{
			BuildStage(T, Stage);
		}
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_CONCEPT_BUILD success count=%d"),
			(int32)UE_ARRAY_COUNT(GConceptStages));
	}

	// ===================== C03: 채택 시안(concept03) 조립 =====================

	struct FC03Textures
	{
		UTexture2D* BoardInterior;
		UTexture2D* RailH;
		UTexture2D* RailVLeft;
		UTexture2D* RailVRight;
		UTexture2D* CornerTL;
		UTexture2D* CornerTR;
		UTexture2D* CornerBL;
		UTexture2D* CornerBR;
		UTexture2D* TitlePlate;
		UTexture2D* StageTab;
		UTexture2D* CoinActive;
		UTexture2D* CoinInactive;
		UTexture2D* BarTrack;
		UTexture2D* BarFill;
		UTexture2D* CtaPlate;
		UTexture2D* ParchWindow;
		UTexture2D* XpBadge;
		UTexture2D* TrackPlate;
		UTexture2D* TrackFill;
		UTexture2D* CardBlank;
		UTexture2D* SelectionGlow;
		UTexture2D* ChestVisual;
		UTexture2D* GoldCoinVisual;
		UTexture2D* Portraits[3];
		UTexture2D* Artifacts[3];
	};

	FC03Textures LoadC03Textures()
	{
		FC03Textures T;
		T.BoardInterior = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_BoardInterior.T_C03_BoardInterior"));
		T.RailH = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailH.T_C03_RailH"));
		T.RailVLeft = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVLeft.T_C03_RailVLeft"));
		T.RailVRight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVRight.T_C03_RailVRight"));
		T.CornerTL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTL.T_C03_CornerTL"));
		T.CornerTR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTR.T_C03_CornerTR"));
		T.CornerBL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBL.T_C03_CornerBL"));
		T.CornerBR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBR.T_C03_CornerBR"));
		T.TitlePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TitlePlate.T_C03_TitlePlate"));
		T.StageTab = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StageTab.T_C03_StageTab"));
		T.CoinActive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinActive.T_C03_StepCoinActive"));
		T.CoinInactive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinInactive.T_C03_StepCoinInactive"));
		T.BarTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarTrack.T_C03_StepBarTrack"));
		T.BarFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarFill.T_C03_StepBarFill"));
		T.CtaPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CtaPlate.T_C03_CtaPlate"));
		T.ParchWindow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_ParchWindow.T_C03_ParchWindow"));
		T.XpBadge = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_XpBadge.T_C03_XpBadge"));
		T.TrackPlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackPlate.T_C03_TrackPlate"));
		T.TrackFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackFill.T_C03_TrackFill"));
		T.CardBlank = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CardBlank.T_C03_CardBlank"));
		T.SelectionGlow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_SelectionGlow.T_C03_SelectionGlow"));
		T.ChestVisual = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_ChestVisual.T_C03_ChestVisual"));
		T.GoldCoinVisual = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_GoldCoinVisual.T_C03_GoldCoinVisual"));
		T.Portraits[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		T.Portraits[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		T.Portraits[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		T.Artifacts[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice"));
		T.Artifacts[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet"));
		T.Artifacts[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"));
		return T;
	}

	void BuildC03Slot(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas, const FC03Textures& T,
		const FConceptSlot& S, int32 CurrentStage)
	{
		int32 PartIndex = 0;
		const FString Base = BaseSlotName(S.Name, PartIndex);
		const FVector2D Origin(S.X, S.Y);
		const FVector2D Size(S.W, S.H);
		const int32 Z = S.Layer * 10;
		const FString Unique = FString(S.Name);

		if (Base == TEXT("backdrop_dim"))
		{
			// 런타임에서는 전투 화면 위 딤. 캡처용은 어두운 바탕만 깐다.
			UBorder* Dim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), FName(*(Unique + TEXT("_border"))));
			Dim->SetBrushColor(FLinearColor(0.020f, 0.024f, 0.030f, 1.f));
			Dim->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Canvas, Dim, Origin, Size, Z);
		}
		else if (Base == TEXT("main_frame"))
		{
			// 목업 힐링 크롭은 이음새가 남아 폐기. 깨끗한 구간 파츠를 9-slice식으로
			// 조립한다: 차콜 인테리어 → 레일 4개 → 코너 브레이스 4개.
			// 보드는 콘텐츠를 담는 바탕이므로 슬롯 layer(strong=3)와 무관하게
			// 항상 최하층(z1~3)에 깐다 — layer*10을 쓰면 콘텐츠를 덮는다.
			AddImage(Blueprint, Canvas, Unique + TEXT("_interior"), T.BoardInterior,
				FVector2D(118.f, 278.f), FVector2D(1300.f, 442.f), 1);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_top"), T.RailH,
				FVector2D(98.f, 258.f), FVector2D(1326.f, 44.f), 2);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_bottom"), T.RailH,
				FVector2D(98.f, 696.f), FVector2D(1326.f, 44.f), 2);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_left"), T.RailVLeft,
				FVector2D(98.f, 258.f), FVector2D(44.f, 482.f), 2);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_right"), T.RailVRight,
				FVector2D(1380.f, 258.f), FVector2D(44.f, 482.f), 2);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_tl"), T.CornerTL,
				FVector2D(98.f, 258.f), FVector2D(92.f, 92.f), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_tr"), T.CornerTR,
				FVector2D(1332.f, 258.f), FVector2D(92.f, 92.f), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_bl"), T.CornerBL,
				FVector2D(98.f, 648.f), FVector2D(92.f, 92.f), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_br"), T.CornerBR,
				FVector2D(1332.f, 648.f), FVector2D(92.f, 92.f), 3);
		}
		else if (Base == TEXT("title_plate"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.TitlePlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("전투 보상"), 42,
				Origin + FVector2D(0.f, Size.Y * 0.2f), FVector2D(Size.X, Size.Y * 0.6f), Z + 1);
		}
		else if (Base == TEXT("step_bar"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_track"), T.BarTrack,
				FVector2D(445.f, 258.f), FVector2D(690.f, 22.f), Z);
			// 진행 필: 활성 코인 중심(552 + 176*n)까지.
			const float ActiveCenter = 552.f + 176.f * CurrentStage;
			AddImage(Blueprint, Canvas, Unique + TEXT("_fill"), T.BarFill,
				FVector2D(452.f, 261.f), FVector2D(ActiveCenter - 452.f, 16.f), Z + 1);
		}
		else if (Base == TEXT("step_coin"))
		{
			const bool bActive = PartIndex == CurrentStage;
			const FVector2D Center = Origin + Size * 0.5f;
			if (bActive)
			{
				AddImage(Blueprint, Canvas, Unique + TEXT("_coin"), T.CoinActive,
					Center - FVector2D(46.f, 46.f), FVector2D(92.f, 92.f), Z + 1);
			}
			else
			{
				AddImage(Blueprint, Canvas, Unique + TEXT("_coin"), T.CoinInactive,
					Origin, Size, Z);
			}
			AddText(Blueprint, Canvas, Unique + TEXT("_num"),
				FString::Printf(TEXT("%d"), PartIndex + 1), bActive ? 30 : 26,
				Center - FVector2D(30.f, bActive ? 24.f : 20.f),
				FVector2D(60.f, bActive ? 46.f : 40.f), Z + 2);
		}
		else if (Base == TEXT("stage_tab"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.StageTab, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), StageKorean(CurrentStage), 24,
				Origin + FVector2D(0.f, Size.Y * 0.1f), FVector2D(Size.X, Size.Y * 0.8f), Z + 1);
		}
		else if (Base == TEXT("cta_button"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CtaPlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				CurrentStage == 3 ? TEXT("확정") : TEXT("다음"), 32,
				Origin + FVector2D(0.f, Size.Y * 0.18f), FVector2D(Size.X, Size.Y * 0.64f), Z + 1);
		}
		else if (Base == TEXT("portrait"))
		{
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_icon"),
				T.Portraits[PartIndex % 3], Origin, Size, Z);
		}
		else if (Base == TEXT("track"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.TrackPlate, Origin, Size, Z);
			const float Ratios[] = { 0.45f, 0.35f, 0.55f };
			AddImage(Blueprint, Canvas, Unique + TEXT("_fill"), T.TrackFill,
				Origin + FVector2D(8.f, 8.f),
				FVector2D((Size.X - 16.f) * Ratios[PartIndex % 3], Size.Y - 16.f), Z + 1);
		}
		else if (Base == TEXT("xp_window") || Base == TEXT("chest_hint_window")
			|| Base == TEXT("gold_window"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.ParchWindow, Origin, Size, Z);
			if (Base == TEXT("chest_hint_window"))
			{
				AddInkText(Blueprint, Canvas, Unique + TEXT("_text"),
					TEXT("상자를 눌러 여세요"), 32,
					Origin + FVector2D(24.f, Size.Y * 0.38f),
					FVector2D(Size.X - 48.f, Size.Y * 0.26f), Z + 1);
			}
			else if (Base == TEXT("gold_window"))
			{
				AddInkText(Blueprint, Canvas, Unique + TEXT("_label"), TEXT("획득 골드"), 34,
					Origin + FVector2D(24.f, Size.Y * 0.2f),
					FVector2D(Size.X - 48.f, Size.Y * 0.26f), Z + 1);
				AddInkText(Blueprint, Canvas, Unique + TEXT("_amount"), TEXT("+350 G"), 52,
					Origin + FVector2D(24.f, Size.Y * 0.5f),
					FVector2D(Size.X - 48.f, Size.Y * 0.34f), Z + 1);
			}
		}
		else if (Base == TEXT("xp_badge"))
		{
			// 배지는 양피지 창(z20) 위에 얹히는 요소다. 슬롯 layer(1) 그대로면
			// 창 뒤에 가려지므로 창 위 고정 z를 쓴다.
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_badge"), T.XpBadge, Origin, Size, 21);
		}
		else if (Base == TEXT("xp_amount_text"))
		{
			AddInkText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("+50 XP"), 44,
				Origin, Size, Z);
		}
		else if (Base == TEXT("levelup_text"))
		{
			AddInkText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("Lv.5  ->  Lv.6"), 30,
				Origin, Size, Z);
		}
		else if (Base == TEXT("chest_visual"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_chest"), T.ChestVisual, Origin, Size, Z);
		}
		else if (Base == TEXT("gold_coin_visual"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_coin"), T.GoldCoinVisual, Origin, Size, Z);
		}
		else if (Base == TEXT("artifact_card"))
		{
			static const TCHAR* Names[] = { TEXT("피의 성배"), TEXT("야수의 송곳니"), TEXT("행운의 주화") };
			AddImage(Blueprint, Canvas, Unique + TEXT("_card"), T.CardBlank, Origin, Size, Z);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_name"), Names[PartIndex % 3], 28,
				Origin + FVector2D(24.f, 26.f), FVector2D(Size.X - 48.f, 52.f), Z + 1);
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_icon"), T.Artifacts[PartIndex % 3],
				Origin + FVector2D(Size.X * 0.19f, Size.Y * 0.28f),
				FVector2D(Size.X * 0.62f, Size.Y * 0.58f), Z + 1);
		}
		else if (Base == TEXT("selection_glow"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_glow"), T.SelectionGlow, Origin, Size, Z + 3);
		}
		else
		{
			checkf(false, TEXT("Unknown c03 slot name: %s"), S.Name);
		}
	}

	void BuildC03Stage(const FC03Textures& T, const FConceptStage& Stage)
	{
		FString StageName = FString(Stage.Stage);
		StageName[0] = FChar::ToUpper(StageName[0]);
		const FString AssetName = FString::Printf(TEXT("WBP_C03_%s"), *StageName);
		const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), PackagePath, *AssetName, *AssetName);

		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
		if (Blueprint == nullptr)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = UUserWidget::StaticClass();
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
				AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
		}
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Could not create c03 board %s"), *AssetName);
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UUserWidget::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("C03Root"));
		Blueprint->WidgetTree->RootWidget = Root;
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("C03Scale"));
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Root->AddChildToOverlay(Scale);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetVerticalAlignment(VAlign_Fill);
		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("C03DesignSize"));
		DesignSize->SetWidthOverride(1536.f);
		DesignSize->SetHeightOverride(864.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("C03Canvas"));
		DesignSize->SetContent(Canvas);

		const int32 Current = StageIndex(Stage.Stage);
		for (int32 Index = 0; Index < Stage.Num; ++Index)
		{
			BuildC03Slot(Blueprint, Canvas, T, Stage.Slots[Index], Current);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()), TEXT("Could not save c03 board %s"), *AssetName);
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_C03_BUILD saved %s"), *AssetName);
	}

	void BuildC03()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_C03_BUILD begin"));
		const FC03Textures T = LoadC03Textures();
		for (const FConceptStage& Stage : GC03Stages)
		{
			BuildC03Stage(T, Stage);
		}
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_C03_BUILD success count=%d"),
			(int32)UE_ARRAY_COUNT(GC03Stages));
	}

	// ===================== BS: 하단 시트 병행 시안 조립 =====================

	struct FBSTextures
	{
		UTexture2D* BattleBackdrop;
		UTexture2D* SheetBackground;
		UTexture2D* RailH;
		UTexture2D* RailVLeft;
		UTexture2D* RailVRight;
		UTexture2D* CornerTL;
		UTexture2D* CornerTR;
		UTexture2D* CornerBL;
		UTexture2D* CornerBR;
		UTexture2D* TitlePlate;
		UTexture2D* StageTab;
		UTexture2D* StepTrack;
		UTexture2D* StepFill;
		UTexture2D* CoinActive;
		UTexture2D* CoinInactive;
		UTexture2D* CtaButton;
		UTexture2D* ParchmentWindow;
		UTexture2D* XpTrack;
		UTexture2D* XpFill;
		UTexture2D* CardBlank;
		UTexture2D* SelectionGlow;
		UTexture2D* ChestBurst;
		UTexture2D* ChestVisual;
		UTexture2D* GoldCoinVisual;
		UTexture2D* XpBadge;
		UTexture2D* Portraits[3];
		UTexture2D* Artifacts[3];
	};

	FBSTextures LoadBSTextures()
	{
		FBSTextures T;
		T.BattleBackdrop = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/BS/T_BS_BattleBackdrop.T_BS_BattleBackdrop"));
		// 자유 생성된 BS 파츠는 품질 상한이 낮아 폐기하고, 목업에서 검증된
		// C03 아트 라이브러리를 공유한다. BS 고유 자산은 전투 배경과 버스트뿐.
		T.SheetBackground = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_BoardInterior.T_C03_BoardInterior"));
		T.RailH = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailH.T_C03_RailH"));
		T.RailVLeft = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVLeft.T_C03_RailVLeft"));
		T.RailVRight = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_RailVRight.T_C03_RailVRight"));
		T.CornerTL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTL.T_C03_CornerTL"));
		T.CornerTR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerTR.T_C03_CornerTR"));
		T.CornerBL = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBL.T_C03_CornerBL"));
		T.CornerBR = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CornerBR.T_C03_CornerBR"));
		T.TitlePlate = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TitlePlate.T_C03_TitlePlate"));
		T.StageTab = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StageTab.T_C03_StageTab"));
		T.StepTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarTrack.T_C03_StepBarTrack"));
		T.StepFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepBarFill.T_C03_StepBarFill"));
		T.CoinActive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinActive.T_C03_StepCoinActive"));
		T.CoinInactive = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_StepCoinInactive.T_C03_StepCoinInactive"));
		T.CtaButton = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CtaPlate.T_C03_CtaPlate"));
		T.ParchmentWindow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_ParchWindow.T_C03_ParchWindow"));
		T.XpTrack = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackPlate.T_C03_TrackPlate"));
		T.XpFill = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_TrackFill.T_C03_TrackFill"));
		T.CardBlank = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_CardBlank.T_C03_CardBlank"));
		T.SelectionGlow = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_SelectionGlow.T_C03_SelectionGlow"));
		T.ChestBurst = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/BS/T_BS_ChestBurst.T_BS_ChestBurst"));
		// BS 시트 인테리어는 C03 차콜과 톤이 달라, 불투명 크롭 배경이 사각
		// 패치로 보인다. 가장자리를 페더링한 변형을 쓴다.
		T.ChestVisual = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_ChestVisualSoft.T_C03_ChestVisualSoft"));
		T.GoldCoinVisual = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_GoldCoinVisualSoft.T_C03_GoldCoinVisualSoft"));
		T.XpBadge = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/ResultBoards/C03/T_C03_XpBadge.T_C03_XpBadge"));
		T.Portraits[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Knight.T_MB_HireIcon_Knight"));
		T.Portraits[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Mage.T_MB_HireIcon_Mage"));
		T.Portraits[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/Mercenaries/T_MB_HireIcon_Rogue.T_MB_HireIcon_Rogue"));
		T.Artifacts[0] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_BloodChalice.T_Artifact_BloodChalice"));
		T.Artifacts[1] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_FangAmulet.T_Artifact_FangAmulet"));
		T.Artifacts[2] = Texture(TEXT("/Game/SVN/OutSideAsset/AICreation/UI/Artifacts/T_Artifact_LuckyCoin.T_Artifact_LuckyCoin"));
		return T;
	}

	void BuildBSSlot(UWidgetBlueprint* Blueprint, UCanvasPanel* Canvas, const FBSTextures& T,
		const FConceptSlot& S, int32 CurrentStage)
	{
		int32 PartIndex = 0;
		const FString Base = BaseSlotName(S.Name, PartIndex);
		const FVector2D Origin(S.X, S.Y);
		const FVector2D Size(S.W, S.H);
		const int32 Z = 10 + S.Layer * 10;
		const FString Unique(S.Name);

		if (Base == TEXT("battle_backdrop"))
		{
			UImage* Backdrop = AddImage(Blueprint, Canvas, Unique, T.BattleBackdrop,
				Origin, Size, 0);
			Backdrop->SetColorAndOpacity(FLinearColor(0.30f, 0.32f, 0.35f, 1.f));
			UBorder* Dim = Blueprint->WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), FName(*(Unique + TEXT("_dim"))));
			Dim->SetBrushColor(FLinearColor(0.015f, 0.020f, 0.025f, 0.60f));
			Dim->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Place(Canvas, Dim, Origin, Size, 1);
		}
		else if (Base == TEXT("sheet_background"))
		{
			// 콘텐츠 컨테이너는 슬롯 layer와 무관하게 항상 최하층이다.
			AddImage(Blueprint, Canvas, Unique, T.SheetBackground, Origin, Size, 2);
		}
		else if (Base == TEXT("sheet_frame"))
		{
			// 자유 생성 프레임 대신 C03 레일·코너를 슬롯 사각형에 9-slice식으로
			// 조립한다 (BuildC03 main_frame과 같은 방식, 크기만 슬롯 기준).
			const float Rail = 44.f;
			const float Corner = 92.f;
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_top"), T.RailH,
				Origin, FVector2D(Size.X, Rail), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_bottom"), T.RailH,
				Origin + FVector2D(0.f, Size.Y - Rail), FVector2D(Size.X, Rail), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_left"), T.RailVLeft,
				Origin, FVector2D(Rail, Size.Y), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_rail_right"), T.RailVRight,
				Origin + FVector2D(Size.X - Rail, 0.f), FVector2D(Rail, Size.Y), 3);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_tl"), T.CornerTL,
				Origin, FVector2D(Corner, Corner), 4);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_tr"), T.CornerTR,
				Origin + FVector2D(Size.X - Corner, 0.f), FVector2D(Corner, Corner), 4);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_bl"), T.CornerBL,
				Origin + FVector2D(0.f, Size.Y - Corner), FVector2D(Corner, Corner), 4);
			AddImage(Blueprint, Canvas, Unique + TEXT("_corner_br"), T.CornerBR,
				Origin + FVector2D(Size.X - Corner, Size.Y - Corner), FVector2D(Corner, Corner), 4);
		}
		else if (Base == TEXT("title_plate"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.TitlePlate, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("전투 보상"), 38,
				Origin + FVector2D(0.f, 13.f), FVector2D(Size.X, 56.f), Z + 1);
		}
		else if (Base == TEXT("stage_tab"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.StageTab, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"), StageKorean(CurrentStage), 23,
				Origin + FVector2D(0.f, 8.f), FVector2D(Size.X, 42.f), Z + 1);
		}
		else if (Base == TEXT("step_track"))
		{
			AddImage(Blueprint, Canvas, Unique, T.StepTrack, Origin, Size, Z);
		}
		else if (Base == TEXT("step_fill"))
		{
			const float Progress = static_cast<float>(CurrentStage + 1) / 4.f;
			AddImage(Blueprint, Canvas, Unique, T.StepFill, Origin,
				FVector2D(Size.X * Progress, Size.Y), Z);
		}
		else if (Base == TEXT("step_coin"))
		{
			const bool bActive = PartIndex == CurrentStage;
			AddImage(Blueprint, Canvas, Unique + TEXT("_coin"),
				bActive ? T.CoinActive : T.CoinInactive, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_number"),
				FString::Printf(TEXT("%d"), PartIndex + 1), bActive ? 28 : 23,
				Origin + FVector2D(0.f, Size.Y * 0.18f),
				FVector2D(Size.X, Size.Y * 0.64f), Z + 1);
		}
		else if (Base == TEXT("cta_button"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_plate"), T.CtaButton, Origin, Size, Z);
			AddText(Blueprint, Canvas, Unique + TEXT("_text"),
				CurrentStage == 3 ? TEXT("확정") : TEXT("다음"), 32,
				Origin + FVector2D(0.f, 16.f), FVector2D(Size.X, 56.f), Z + 1);
		}
		else if (Base == TEXT("portrait"))
		{
			AddAspectImage(Blueprint, Canvas, Unique, T.Portraits[PartIndex % 3],
				Origin, Size, Z);
		}
		else if (Base == TEXT("xp_gauge"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_track"), T.XpTrack, Origin, Size, Z);
			const float Ratios[] = { 0.46f, 0.37f, 0.55f };
			AddImage(Blueprint, Canvas, Unique + TEXT("_fill"), T.XpFill,
				Origin + FVector2D(8.f, 8.f),
				FVector2D((Size.X - 16.f) * Ratios[PartIndex % 3], Size.Y - 16.f), Z + 1);
		}
		else if (Base == TEXT("xp_summary_window"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.ParchmentWindow,
				Origin, Size, Z);
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_badge"), T.XpBadge,
				Origin + FVector2D(175.f, 26.f), FVector2D(140.f, 126.f), Z + 1);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_amount"), TEXT("+50 XP"), 42,
				Origin + FVector2D(45.f, 160.f), FVector2D(Size.X - 90.f, 54.f), Z + 2);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_level"), TEXT("Lv.5  ->  Lv.6"), 28,
				Origin + FVector2D(45.f, 222.f), FVector2D(Size.X - 90.f, 44.f), Z + 2);
		}
		else if (Base == TEXT("chest_burst"))
		{
			AddImage(Blueprint, Canvas, Unique, T.ChestBurst, Origin, Size, Z);
		}
		else if (Base == TEXT("chest_visual"))
		{
			AddAspectImage(Blueprint, Canvas, Unique, T.ChestVisual, Origin, Size, Z);
		}
		else if (Base == TEXT("chest_hint_window"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.ParchmentWindow,
				Origin, Size, Z);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_text"), TEXT("상자를 눌러 여세요"), 34,
				Origin + FVector2D(28.f, 100.f), FVector2D(Size.X - 56.f, 70.f), Z + 1);
		}
		else if (Base == TEXT("gold_coin_visual"))
		{
			AddAspectImage(Blueprint, Canvas, Unique, T.GoldCoinVisual, Origin, Size, Z);
		}
		else if (Base == TEXT("gold_window"))
		{
			AddImage(Blueprint, Canvas, Unique + TEXT("_window"), T.ParchmentWindow,
				Origin, Size, Z);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_label"), TEXT("획득 골드"), 35,
				Origin + FVector2D(36.f, 60.f), FVector2D(Size.X - 72.f, 62.f), Z + 1);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_amount"), TEXT("+350 G"), 54,
				Origin + FVector2D(36.f, 135.f), FVector2D(Size.X - 72.f, 82.f), Z + 1);
		}
		else if (Base == TEXT("artifact_card"))
		{
			static const TCHAR* Names[] = { TEXT("피의 성배"), TEXT("야수의 송곳니"), TEXT("행운의 주화") };
			AddImage(Blueprint, Canvas, Unique + TEXT("_card"), T.CardBlank, Origin, Size, Z);
			AddInkText(Blueprint, Canvas, Unique + TEXT("_name"), Names[PartIndex % 3], 27,
				Origin + FVector2D(24.f, 30.f), FVector2D(Size.X - 48.f, 48.f), Z + 1);
			AddAspectImage(Blueprint, Canvas, Unique + TEXT("_icon"), T.Artifacts[PartIndex % 3],
				Origin + FVector2D(52.f, 104.f), FVector2D(Size.X - 104.f, 176.f), Z + 1);
		}
		else if (Base == TEXT("selection_glow"))
		{
			AddImage(Blueprint, Canvas, Unique, T.SelectionGlow, Origin, Size, Z + 3);
		}
		else
		{
			checkf(false, TEXT("Unknown BS slot name: %s"), S.Name);
		}
	}

	void BuildBSStage(const FBSTextures& T, const FConceptStage& Stage)
	{
		FString StageName(Stage.Stage);
		StageName[0] = FChar::ToUpper(StageName[0]);
		const FString AssetName = FString::Printf(TEXT("WBP_BS_%s"), *StageName);
		const FString AssetPath = FString::Printf(TEXT("%s/%s.%s"), PackagePath, *AssetName, *AssetName);

		UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr, *AssetPath);
		if (Blueprint == nullptr)
		{
			UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
			Factory->ParentClass = UUserWidget::StaticClass();
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Blueprint = Cast<UWidgetBlueprint>(AssetTools.Get().CreateAsset(
				AssetName, PackagePath, UWidgetBlueprint::StaticClass(), Factory));
		}
		checkf(Blueprint != nullptr && Blueprint->WidgetTree != nullptr,
			TEXT("Could not create BS board %s"), *AssetName);
		Blueprint->Modify();
		Blueprint->WidgetTree->Modify();
		if (UWidget* PreviousRoot = Blueprint->WidgetTree->RootWidget)
		{
			TSet<UWidget*> PreviousWidgets;
			PreviousWidgets.Add(PreviousRoot);
			FWidgetBlueprintEditorUtils::DeleteWidgets(Blueprint, MoveTemp(PreviousWidgets),
				FWidgetBlueprintEditorUtils::EDeleteWidgetWarningType::DeleteSilently);
		}
		Blueprint->ParentClass = UUserWidget::StaticClass();

		UOverlay* Root = Blueprint->WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("BSRoot"));
		Blueprint->WidgetTree->RootWidget = Root;
		UScaleBox* Scale = Blueprint->WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("BSScale"));
		Scale->SetStretch(EStretch::ScaleToFit);
		Scale->SetStretchDirection(EStretchDirection::Both);
		Root->AddChildToOverlay(Scale);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetHorizontalAlignment(HAlign_Fill);
		CastChecked<UOverlaySlot>(Scale->Slot)->SetVerticalAlignment(VAlign_Fill);
		USizeBox* DesignSize = Blueprint->WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("BSDesignSize"));
		DesignSize->SetWidthOverride(1536.f);
		DesignSize->SetHeightOverride(864.f);
		Scale->AddChild(DesignSize);
		UCanvasPanel* Canvas = Blueprint->WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("BSCanvas"));
		DesignSize->SetContent(Canvas);

		const int32 Current = StageIndex(Stage.Stage);
		for (int32 Index = 0; Index < Stage.Num; ++Index)
		{
			BuildBSSlot(Blueprint, Canvas, T, Stage.Slots[Index], Current);
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Blueprint->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		checkf(UPackage::SavePackage(Blueprint->GetPackage(), Blueprint, *Filename,
			FSavePackageArgs()), TEXT("Could not save BS board %s"), *AssetName);
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_BS_BUILD saved %s"), *AssetName);
	}

	void BuildBS()
	{
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_BS_BUILD begin"));
		const FBSTextures T = LoadBSTextures();
		for (const FConceptStage& Stage : GBSStages)
		{
			BuildBSStage(T, Stage);
		}
		UE_LOG(LogTemp, Display, TEXT("RD_REWARD_BS_BUILD success count=%d"),
			(int32)UE_ARRAY_COUNT(GBSStages));
	}
}

void RegisterRewardConceptBoardBuilderCommands()
{
	using namespace RewardConceptBoardBuilder;
	BuildCommand = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.BuildRewardConceptBoards"),
		TEXT("Build reward flow concept comparison boards (concepts 02/03/06 x 4 stages)."),
		FConsoleCommandDelegate::CreateStatic(&Build));
	BuildC03Command = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.BuildRewardC03Boards"),
		TEXT("Build the chosen concept03 reward boards from mockup-derived parts."),
		FConsoleCommandDelegate::CreateStatic(&BuildC03));
	BuildBSCommand = MakeUnique<FAutoConsoleCommand>(TEXT("RD.Editor.BuildRewardBSBoards"),
		TEXT("Build the bottom-sheet reward boards from BS atomic parts."),
		FConsoleCommandDelegate::CreateStatic(&BuildBS));
}

void UnregisterRewardConceptBoardBuilderCommands()
{
	RewardConceptBoardBuilder::BuildCommand.Reset();
	RewardConceptBoardBuilder::BuildC03Command.Reset();
	RewardConceptBoardBuilder::BuildBSCommand.Reset();
}
