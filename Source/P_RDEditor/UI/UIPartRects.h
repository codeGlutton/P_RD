// 이 파일은 만들어진 것이다. 손으로 고치지 말 것.
//   Tools/UI/export_inner_rects.py 가 다시 만든다.
//
// 값의 출처는 사람이 목록 페이지(Tools/UI/mockups/assets.html)에서 눈으로 맞춘
// **칸**이다 -- 그림 안에 글자·아이콘을 놓아도 테두리를 안 밟는 자리.
// 안 맞춘 것은 잰 값(measure_all_assets.py)이다.

#pragma once

#include "CoreMinimal.h"

namespace UIPartRects
{
	/** @brief 그림 한 장의 잰 값. 비율은 **원본 텍스처 기준**이다. */
	struct FPart
	{
		const TCHAR* Name;
		int32 Index;                      // 칸이 여럿인 그림이 있다(머리칸 + 몸통)
		float Left, Top, Right, Bottom;   // 쓸 수 있는 칸 (0~1)
		float SourceW, SourceH;           // 그 비율을 잰 원본 크기
	};

	static const FPart Parts[] = {
		{ TEXT("T_KitA_Button_Small_Normal"), 0, 0.1195f, 0.2145f, 0.8706f, 0.7876f, 281.f, 133.f },   // 사람
		{ TEXT("T_KitA_Cell_Normal"), 0, 0.0936f, 0.0951f, 0.9091f, 0.9005f, 186.f, 176.f },   // 사람
		{ TEXT("T_KitA_Frame_Outer"), 0, 0.0365f, 0.0741f, 0.9635f, 0.9352f, 1920.f, 1080.f },   // 잼
		{ TEXT("T_MB_ArtifactSlot_Frame"), 0, 0.1545f, 0.1512f, 0.8433f, 0.8408f, 1254.f, 1254.f },   // 사람
		{ TEXT("T_MB_Defeat_ButtonPrimary"), 0, 0.1228f, 0.3196f, 0.8741f, 0.6539f, 1821.f, 864.f },   // 사람
		{ TEXT("T_MB_Defeat_ButtonSecondary"), 0, 0.1255f, 0.3054f, 0.8744f, 0.6993f, 1916.f, 821.f },   // 사람
		{ TEXT("T_MB_Defeat_MercenaryCard"), 0, 0.2010f, 0.1807f, 0.7990f, 0.6806f, 1122.f, 1402.f },   // 사람
		{ TEXT("T_MB_Defeat_MercenaryCard"), 1, 0.2149f, 0.6957f, 0.7908f, 0.8078f, 1122.f, 1402.f },   // 사람
		{ TEXT("T_MB_Defeat_OuterFrame"), 0, 0.1752f, 0.1505f, 0.8255f, 0.7781f, 1086.f, 1448.f },   // 사람
		{ TEXT("T_MB_Defeat_TitleBanner"), 0, 0.2172f, 0.3363f, 0.7947f, 0.5771f, 1862.f, 845.f },   // 사람
		{ TEXT("T_MB_GenericDetailPanel"), 0, 0.1180f, 0.2218f, 0.8861f, 0.7704f, 1536.f, 1024.f },   // 사람
		{ TEXT("T_MB_GenericDetailPanel"), 1, 0.2722f, 0.0828f, 0.7303f, 0.1800f, 1536.f, 1024.f },   // 사람
		{ TEXT("T_MB_HireBackButton"), 0, 0.0858f, 0.1822f, 0.9183f, 0.7811f, 768.f, 305.f },   // 사람
		{ TEXT("T_MB_HireDepartButton"), 0, 0.0864f, 0.1880f, 0.9082f, 0.7834f, 768.f, 364.f },   // 사람
		{ TEXT("T_MB_HireHero_Knight"), 0, 0.1198f, 0.1198f, 0.8802f, 0.8802f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Barbarian"), 0, 0.2500f, 0.1719f, 0.7500f, 0.8906f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Druid"), 0, 0.1510f, 0.1667f, 0.8438f, 0.8906f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Knight"), 0, 0.3438f, 0.2552f, 0.7188f, 0.8594f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Mage"), 0, 0.1927f, 0.1510f, 0.8333f, 0.8750f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Ranger"), 0, 0.2448f, 0.2656f, 0.7604f, 0.8958f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireIcon_Rogue"), 0, 0.2083f, 0.1458f, 0.8021f, 0.8854f, 1254.f, 1254.f },   // 잼
		{ TEXT("T_MB_HireListFrame"), 0, 0.0754f, 0.0384f, 0.9236f, 0.9583f, 512.f, 884.f },   // 사람
		{ TEXT("T_MB_HireNamePlate"), 0, 0.0305f, 0.1260f, 0.9717f, 0.8470f, 1024.f, 237.f },   // 사람
		{ TEXT("T_MB_HirePartyFrame"), 0, 0.0776f, 0.1136f, 0.9266f, 0.9488f, 640.f, 956.f },   // 사람
		{ TEXT("T_MB_HirePartyFrame"), 1, 0.1270f, 0.0364f, 0.8764f, 0.1029f, 640.f, 956.f },   // 사람
		{ TEXT("T_MB_HirePartyRowEmpty"), 0, 0.0481f, 0.1103f, 0.9534f, 0.8621f, 1024.f, 420.f },   // 사람
		{ TEXT("T_MB_HirePartyRowPlus"), 0, 0.0524f, 0.1275f, 0.9521f, 0.8627f, 1024.f, 420.f },   // 사람
		{ TEXT("T_MB_HireRowNormal"), 0, 0.0363f, 0.0878f, 0.9613f, 0.8961f, 1024.f, 288.f },   // 사람
		{ TEXT("T_MB_HireRowSelected"), 0, 0.0469f, 0.1159f, 0.9558f, 0.8853f, 1024.f, 288.f },   // 사람
		{ TEXT("T_MB_HireSkillButtonFrame"), 0, 0.1502f, 0.1020f, 0.8290f, 0.8818f, 1254.f, 1254.f },   // 사람
		{ TEXT("T_MB_HireStatsStrip"), 0, 0.0190f, 0.1122f, 0.9760f, 0.8411f, 1400.f, 196.f },   // 사람
		{ TEXT("T_MB_HireTitlePlate"), 0, 0.0541f, 0.1772f, 0.9444f, 0.8289f, 1024.f, 254.f },   // 사람
		{ TEXT("T_MB_Icon_Speed"), 0, 0.1406f, 0.1453f, 0.8438f, 0.8547f, 1067.f, 960.f },   // 잼
		{ TEXT("T_MB_MercenaryCard_Normal"), 0, 0.0968f, 0.1488f, 0.9012f, 0.8486f, 1644.f, 957.f },   // 사람
		{ TEXT("T_MB_MercenaryCard_Selected"), 0, 0.0957f, 0.1473f, 0.9028f, 0.8536f, 1644.f, 957.f },   // 사람
		{ TEXT("T_MB_OptionsIcon_Map"), 0, 0.4115f, 0.3933f, 0.5365f, 0.6067f, 1029.f, 958.f },   // 잼
		{ TEXT("T_MB_OptionsIcon_MercenaryGlyph"), 0, 0.1200f, 0.1198f, 0.8800f, 0.5781f, 808.f, 1234.f },   // 잼
		{ TEXT("T_MB_OptionsIcon_MonsterGlyph"), 0, 0.1173f, 0.1198f, 0.8827f, 0.8802f, 1058.f, 1131.f },   // 잼
		{ TEXT("T_MB_OptionsIcon_Settings"), 0, 0.4115f, 0.4127f, 0.5885f, 0.5873f, 1039.f, 1027.f },   // 잼
		{ TEXT("T_MB_OptionsRail_Frame"), 0, 0.0910f, 0.2123f, 0.2665f, 0.7996f, 1554.f, 571.f },   // 사람
		{ TEXT("T_MB_OptionsRail_Frame"), 1, 0.3060f, 0.2034f, 0.4793f, 0.7966f, 1554.f, 571.f },   // 사람
		{ TEXT("T_MB_OptionsRail_Frame"), 2, 0.5222f, 0.2061f, 0.6956f, 0.8001f, 1554.f, 571.f },   // 사람
		{ TEXT("T_MB_OptionsRail_Frame"), 3, 0.7345f, 0.2061f, 0.9094f, 0.7952f, 1554.f, 571.f },   // 사람
		{ TEXT("T_MB_RoundBadge_Frame"), 0, 0.1260f, 0.2228f, 0.8735f, 0.7805f, 1839.f, 573.f },   // 사람
		{ TEXT("T_MB_StatusSlot_Frame"), 0, 0.1397f, 0.1328f, 0.8592f, 0.8574f, 1254.f, 1254.f },   // 사람
		{ TEXT("T_MB_TurnToken_Frame"), 0, 0.1566f, 0.1385f, 0.8389f, 0.6610f, 731.f, 995.f },   // 사람
		{ TEXT("T_MB_TurnToken_Frame"), 1, 0.1539f, 0.7071f, 0.8430f, 0.8654f, 731.f, 995.f },   // 사람
		{ TEXT("T_MT_RowNormal"), 0, 0.0521f, 0.1471f, 0.9479f, 0.8529f, 1903.f, 681.f },   // 잼
		{ TEXT("T_MT_RowSelected"), 0, 0.0573f, 0.1618f, 0.9479f, 0.8529f, 1903.f, 681.f },   // 잼
		{ TEXT("T_RS_ExpFrame"), 0, 0.0625f, 0.2000f, 0.9427f, 0.8000f, 1650.f, 953.f },   // 잼
		{ TEXT("T_RS_Step1Active"), 0, 0.1302f, 0.4219f, 0.8698f, 0.5781f, 2172.f, 724.f },   // 잼
		{ TEXT("T_RS_VictoryPanelNeutral"), 0, 0.1831f, 0.1615f, 0.8169f, 0.8490f, 1081.f, 1455.f },   // 잼
	};

	/**
	 * @brief 그림을 (X,Y,W,H) 에 놓았을 때 **안에 글자를 넣어도 되는 사각**.
	 *
	 * @param bSliced true 면 9-slice(Box)로 그린다는 뜻이다. 그때 테두리는
	 *                늘어나지 않고 **원본 픽셀 크기 그대로** 그려지므로
	 *                (ElementBatcher.cpp:857), 안쪽 자리도 가장자리에서 늘 같은
	 *                픽셀만큼 떨어져 있다. 비율을 그대로 곱하면 크게 틀린다.
	 *                통짜(Image)로 그리면 전체가 같은 배율이라 비율이 맞다.
	 * @return 못 찾으면 사방 12% 를 들인 사각. 짐작이지만 한 곳에만 둔다.
	 */
	inline FBox2D Inner(const TCHAR* Name, const FVector2D Position,
		const FVector2D Size, const bool bSliced = true, const int32 Which = 0)
	{
		for (const FPart& Part : Parts)
		{
			if (FCString::Strcmp(Part.Name, Name) != 0 || Part.Index != Which)
			{
				continue;
			}
			if (bSliced == false)
			{
				return FBox2D(
					Position + FVector2D(Size.X * Part.Left, Size.Y * Part.Top),
					Position + FVector2D(Size.X * Part.Right, Size.Y * Part.Bottom));
			}
			FVector2D Near(Part.Left * Part.SourceW, Part.Top * Part.SourceH);
			FVector2D Far((1.f - Part.Right) * Part.SourceW,
				(1.f - Part.Bottom) * Part.SourceH);
			// 놓을 자리가 원본보다 좁으면 테두리끼리 겹친다. 엔진도 그때는
			// 반씩으로 자르므로 여기서도 같은 비율로 줄인다.
			if (Near.X + Far.X > Size.X * 0.8f)
			{
				const float Shrink = Size.X * 0.8f / (Near.X + Far.X);
				Near.X *= Shrink;
				Far.X *= Shrink;
			}
			if (Near.Y + Far.Y > Size.Y * 0.8f)
			{
				const float Shrink = Size.Y * 0.8f / (Near.Y + Far.Y);
				Near.Y *= Shrink;
				Far.Y *= Shrink;
			}
			return FBox2D(Position + Near, Position + Size - Far);
		}
		const FVector2D Guess = Size * 0.12f;
		return FBox2D(Position + Guess, Position + Size - Guess);
	}

	/** @brief 이 그림에 사람이 그어 둔 칸이 몇 개인가. */
	inline int32 Count(const TCHAR* Name)
	{
		int32 Found = 0;
		for (const FPart& Part : Parts)
		{
			if (FCString::Strcmp(Part.Name, Name) == 0)
			{
				++Found;
			}
		}
		return Found;
	}

	/**
	 * @brief 여러 칸으로 나눠 쓰는 그림의 Which 번째 칸.
	 *
	 * @details
	 * **그어 둔 칸이 있으면 그것을 쓴다.** 그림에 칸막이가 그려져 있으면 사람이
	 * 그 수만큼 그어 두었을 것이고, 그게 정본이다. 기계가 그림에서 칸막이를
	 * 찾아내는 짓은 하지 않는다 -- 재는 곳이 둘이 되면 어느 쪽이 맞는지 따지게
	 * 되고, 사람이 손으로 다 맞춰 둔 뜻이 없어진다.
	 *
	 * 하나만 그어져 있으면 그 하나를 Total 등분한다. 칸막이가 없는 그림이거나,
	 * 아직 안 나눠 그은 그림이다. 어느 쪽이든 칸을 더 그으면 여기가 따라온다.
	 */
	inline FBox2D Cell(const TCHAR* Name, const FVector2D Position,
		const FVector2D Size, const bool bSliced, const int32 Which,
		const int32 Total, const bool bHorizontal = true)
	{
		if (Count(Name) >= Total)
		{
			return Inner(Name, Position, Size, bSliced, Which);
		}
		const FBox2D Whole = Inner(Name, Position, Size, bSliced, 0);
		const FVector2D Span = Whole.GetSize();
		if (bHorizontal)
		{
			const float Step = Span.X / Total;
			return FBox2D(Whole.Min + FVector2D(Step * Which, 0.f),
				Whole.Min + FVector2D(Step * (Which + 1), Span.Y));
		}
		const float Step = Span.Y / Total;
		return FBox2D(Whole.Min + FVector2D(0.f, Step * Which),
			Whole.Min + FVector2D(Span.X, Step * (Which + 1)));
	}
}
