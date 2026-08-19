// Scripts/Editor/gen_bs_slots.py가 bs-slots.json에서 생성.
// 손으로 수정하지 말 것. 좌표 수정은 JSON을 고치고 재생성.
// clang-format off

static const FConceptSlot GSlotsBS_experience[] =
{
	{ TEXT("battle_backdrop"), 0.f, 0.f, 1536.f, 170.f, 0, 0, false },
	{ TEXT("sheet_background"), 32.f, 170.f, 1472.f, 694.f, 0, 0, false },
	{ TEXT("sheet_frame"), 32.f, 170.f, 1472.f, 694.f, 1, 3, false },
	{ TEXT("title_plate"), 80.f, 194.f, 360.f, 84.f, 2, 2, true },
	{ TEXT("step_track"), 500.f, 218.f, 760.f, 22.f, 2, 1, false },
	{ TEXT("step_fill"), 508.f, 223.f, 744.f, 12.f, 3, 0, false },
	{ TEXT("step_coin_1"), 500.f, 191.f, 76.f, 76.f, 4, 1, true },
	{ TEXT("step_coin_2"), 716.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_3"), 928.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_4"), 1140.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("stage_tab"), 1300.f, 197.f, 160.f, 58.f, 2, 1, true },
	{ TEXT("cta_button"), 588.f, 760.f, 360.f, 88.f, 2, 2, true },
	{ TEXT("portrait_1"), 142.f, 340.f, 96.f, 96.f, 2, 0, false },
	{ TEXT("xp_gauge_1"), 264.f, 368.f, 330.f, 40.f, 2, 1, false },
	{ TEXT("portrait_2"), 142.f, 472.f, 96.f, 96.f, 2, 0, false },
	{ TEXT("xp_gauge_2"), 264.f, 500.f, 330.f, 40.f, 2, 1, false },
	{ TEXT("portrait_3"), 142.f, 604.f, 96.f, 96.f, 2, 0, false },
	{ TEXT("xp_gauge_3"), 264.f, 632.f, 330.f, 40.f, 2, 1, false },
	{ TEXT("xp_summary_window"), 748.f, 350.f, 490.f, 300.f, 2, 2, true },
};

static const FConceptSlot GSlotsBS_chest[] =
{
	{ TEXT("battle_backdrop"), 0.f, 0.f, 1536.f, 170.f, 0, 0, false },
	{ TEXT("sheet_background"), 32.f, 170.f, 1472.f, 694.f, 0, 0, false },
	{ TEXT("sheet_frame"), 32.f, 170.f, 1472.f, 694.f, 1, 3, false },
	{ TEXT("title_plate"), 80.f, 194.f, 360.f, 84.f, 2, 2, true },
	{ TEXT("step_track"), 500.f, 218.f, 760.f, 22.f, 2, 1, false },
	{ TEXT("step_fill"), 508.f, 223.f, 744.f, 12.f, 3, 0, false },
	{ TEXT("step_coin_1"), 506.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_2"), 710.f, 191.f, 76.f, 76.f, 4, 1, true },
	{ TEXT("step_coin_3"), 928.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_4"), 1140.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("stage_tab"), 1300.f, 197.f, 160.f, 58.f, 2, 1, true },
	{ TEXT("cta_button"), 588.f, 760.f, 360.f, 88.f, 2, 2, true },
	{ TEXT("chest_burst"), 280.f, 326.f, 420.f, 360.f, 2, 0, false },
	{ TEXT("chest_visual"), 318.f, 344.f, 344.f, 324.f, 3, 0, false },
	{ TEXT("chest_hint_window"), 820.f, 388.f, 500.f, 282.f, 2, 2, true },
};

static const FConceptSlot GSlotsBS_gold[] =
{
	{ TEXT("battle_backdrop"), 0.f, 0.f, 1536.f, 170.f, 0, 0, false },
	{ TEXT("sheet_background"), 32.f, 170.f, 1472.f, 694.f, 0, 0, false },
	{ TEXT("sheet_frame"), 32.f, 170.f, 1472.f, 694.f, 1, 3, false },
	{ TEXT("title_plate"), 80.f, 194.f, 360.f, 84.f, 2, 2, true },
	{ TEXT("step_track"), 500.f, 218.f, 760.f, 22.f, 2, 1, false },
	{ TEXT("step_fill"), 508.f, 223.f, 744.f, 12.f, 3, 0, false },
	{ TEXT("step_coin_1"), 506.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_2"), 716.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_3"), 922.f, 191.f, 76.f, 76.f, 4, 1, true },
	{ TEXT("step_coin_4"), 1140.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("stage_tab"), 1300.f, 197.f, 160.f, 58.f, 2, 1, true },
	{ TEXT("cta_button"), 588.f, 760.f, 360.f, 88.f, 2, 2, true },
	{ TEXT("gold_coin_visual"), 300.f, 380.f, 300.f, 300.f, 2, 0, false },
	{ TEXT("gold_window"), 760.f, 390.f, 500.f, 282.f, 2, 2, true },
};

static const FConceptSlot GSlotsBS_artifact[] =
{
	{ TEXT("battle_backdrop"), 0.f, 0.f, 1536.f, 170.f, 0, 0, false },
	{ TEXT("sheet_background"), 32.f, 170.f, 1472.f, 694.f, 0, 0, false },
	{ TEXT("sheet_frame"), 32.f, 170.f, 1472.f, 694.f, 1, 3, false },
	{ TEXT("title_plate"), 80.f, 194.f, 360.f, 84.f, 2, 2, true },
	{ TEXT("step_track"), 500.f, 218.f, 760.f, 22.f, 2, 1, false },
	{ TEXT("step_fill"), 508.f, 223.f, 744.f, 12.f, 3, 0, false },
	{ TEXT("step_coin_1"), 506.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_2"), 716.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_3"), 928.f, 197.f, 64.f, 64.f, 4, 1, true },
	{ TEXT("step_coin_4"), 1134.f, 191.f, 76.f, 76.f, 4, 1, true },
	{ TEXT("stage_tab"), 1300.f, 197.f, 160.f, 58.f, 2, 1, true },
	{ TEXT("cta_button"), 588.f, 760.f, 360.f, 88.f, 2, 2, true },
	{ TEXT("artifact_card_1"), 214.f, 344.f, 280.f, 320.f, 2, 2, true },
	{ TEXT("artifact_card_2"), 628.f, 344.f, 280.f, 320.f, 2, 2, true },
	{ TEXT("artifact_card_3"), 1042.f, 344.f, 280.f, 320.f, 2, 2, true },
	{ TEXT("selection_glow"), 622.f, 338.f, 292.f, 332.f, 3, 1, false },
};

static const FConceptStage GBSStages[] =
{
	{ TEXT("bs_bottom_sheet"), TEXT("experience"), GSlotsBS_experience, UE_ARRAY_COUNT(GSlotsBS_experience) },
	{ TEXT("bs_bottom_sheet"), TEXT("chest"), GSlotsBS_chest, UE_ARRAY_COUNT(GSlotsBS_chest) },
	{ TEXT("bs_bottom_sheet"), TEXT("gold"), GSlotsBS_gold, UE_ARRAY_COUNT(GSlotsBS_gold) },
	{ TEXT("bs_bottom_sheet"), TEXT("artifact"), GSlotsBS_artifact, UE_ARRAY_COUNT(GSlotsBS_artifact) },
};
// clang-format on
