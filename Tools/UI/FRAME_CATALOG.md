# 프레임 에셋 카탈로그 — 각 프레임이 강제하는 칸

`Tools/UI/catalog_frame_regions.py` 가 그림의 밝기 프로파일에서 분할선을 찾아
**콘텐츠를 놓을 수 있는 칸을 비율(0~1)로** 적은 것이다. 비율이라 어떤 크기로
늘려 붙여도 그대로 환산해 쓸 수 있다.

> 프레임을 고를 때는 **화면에 필요한 칸 수**를 먼저 정하고 그 칸을 가진 프레임을
> 골라야 한다. 반대로 하면(프레임 먼저, 배치 나중) 지금처럼 섹션이 분할선을 밟는다.

## 3열 전면 판  (1종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_MT_BaseFrame` | MonsterTab | 1672×941 | 1.78 | 0.02~0.26 | 0.27~0.56 | 0.57~0.98 | 0.12~0.94 |

## 2열 전면 판  (2종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_Hire_BoardShell_V08` | Hire | 1770×1472 | 1.20 | 0.04~0.12 | 0.13~0.88 | 0.18~0.77 |
| `KK_HUD04_status_slot_frame` | HUD04 | 851×1024 | 0.83 | 0.01~0.10 | 0.88~0.97 | 0.01~0.08 | 0.76~0.84 |

## 단일 판  (23종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_Hire_Background_V09` | Hire | 2048×1152 | 1.78 | 0.44~0.81 | 0.08~0.79 |
| `M_reward_v02_vertical_cards` | Mockups | 1920×1080 | 1.78 | - | - |
| `T_detail_scrim` | CombatDetail | 1672×941 | 1.78 | 0.17~0.83 | 0.14~0.81 |
| `T_MercenaryRoster_Shell` | Combat | 1672×941 | 1.78 | 0.54~0.98 | 0.19~0.92 |
| `T_MB_Defeat_BattleSummary` | Defeat | 1672×941 | 1.78 | 0.09~0.91 | 0.19~0.81 |
| `T_Inventory_Background_Current` | RunFlow | 1672×941 | 1.78 | - | 0.02~0.17 |
| `T_set_scrim` | Settings | 1672×941 | 1.78 | 0.17~0.81 | 0.15~0.78 |
| `T_title_bg` | Title | 1672×941 | 1.78 | 0.18~0.83 | 0.48~0.94 |
| `T_wm_panel_scrim` | WorldMap | 1672×941 | 1.78 | 0.17~0.83 | 0.14~0.81 |
| `T_Inventory_Background_Current` | RunFlow | 1672×941 | 1.78 | - | 0.02~0.17 |
| `T_MB_MercenaryCard_Normal` | Combat | 1644×957 | 1.72 | - | - |
| `T_MB_MercenaryCard_Selected` | Combat | 1644×957 | 1.72 | - | - |
| `T_CombatHUD_SkillFrame_Transparent` | CombatHUD | 1536×1024 | 1.50 | - | - |
| `T_MB_GenericDetailPanel` | Common | 1536×1024 | 1.50 | - | 0.06~0.21 |
| `T_RS_ExpFrame` | RewardSettlement | 1650×953 | 1.73 | 0.04~0.96 | 0.06~0.87 |
| `KK_HUD04_hero_status_frame` | HUD04 | 1354×808 | 1.68 | - | - |
| `T_CombatHUD_SkillDetailFrame` | CombatHUD | 1407×718 | 1.96 | - | - |
| `T_Hire_InfoPanel_V11` | Hire | 1184×820 | 1.44 | - | - |
| `T_modal_panel_frame` | ClassSelect | 1107×821 | 1.35 | - | - |
| `T_set_panel_frame` | Settings | 1107×821 | 1.35 | - | - |
| `T_Reward_ArtifactCard_V2` | Reward | 458×553 | 0.83 | - | - |
| `T_Reward_ArtifactCardRare_V2` | Reward | 458×553 | 0.83 | - | - |
| `T_Reward_ArtifactCardSel_V2` | Reward | 458×553 | 0.83 | - | - |

## 2단 판  (4종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_Reward_Background_Current` | RunFlow | 1672×941 | 1.78 | 0.88~0.97 | 0.09~0.17 | 0.82~0.89 |
| `T_SetTheme_Panel` | SettingsTheme_Temp | 1717×916 | 1.87 | 0.09~0.91 | 0.40~0.57 | 0.67~0.74 |
| `T_detail_panel_frame` | CombatDetail | 1403×1121 | 1.25 | - | 0.00~0.10 | 0.90~1.00 |
| `T_MB_HirePartyRowPlus` | Hire | 1024×420 | 2.44 | 0.02~0.98 | 0.03~0.10 | 0.11~0.94 |

## 세로 기둥 (명단/파티 열)  (21종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_StageMap_Background_Parchment` | Map | 941×1672 | 0.56 | 0.17~0.85 | 0.10~0.94 |
| `T_StageMap_Background_Current` | RunFlow | 941×1672 | 0.56 | 0.19~0.82 | 0.04~0.99 |
| `T_MB_Defeat_MercenaryCard` | Defeat | 1122×1402 | 0.80 | - | - |
| `T_RS_VictoryPanelNeutral` | RewardSettlement | 1081×1455 | 0.74 | - | - |
| `T_MB_Defeat_OuterFrame` | Defeat | 1086×1448 | 0.75 | 0.16~0.83 | 0.14~0.79 |
| `T_Hire_DetailPanel_V08` | Hire | 770×1472 | 0.52 | - | - |
| `KK_HUD04_party_card_wood` | HUD04 | 630×1453 | 0.43 | 0.01~0.10 | 0.88~0.98 | - |
| `KK_HUD04_plate_bottom_status_center` | HUD04 | 630×1453 | 0.43 | 0.01~0.10 | 0.88~0.98 | - |
| `KK_HUD04_plate_bottom_status_left` | HUD04 | 630×1453 | 0.43 | 0.01~0.10 | 0.88~0.98 | - |
| `KK_HUD04_plate_bottom_status_right` | HUD04 | 630×1453 | 0.43 | 0.01~0.10 | 0.88~0.98 | - |
| `T_Hire_Card_V11` | Hire | 693×1188 | 0.58 | - | - |
| `T_MB_TurnToken_Frame` | Combat | 731×995 | 0.73 | - | - |
| `T_Hire_CardSelected_V11` | Hire | 660×1102 | 0.60 | - | - |
| `T_MB_HirePartyFrame` | Hire | 640×956 | 0.67 | - | 0.04~0.11 |
| `T_MB_HireListFrame` | Hire | 512×884 | 0.58 | - | - |
| `T_Hire_ListPanel_V12` | Hire | 581×753 | 0.77 | - | - |
| `T_Reward_ArtifactCardSel_V1` | Reward | 544×800 | 0.68 | 0.11~0.89 | 0.08~0.96 |
| `T_Reward_ArtifactCard_V1` | Reward | 512×768 | 0.67 | 0.09~0.91 | 0.06~0.98 |
| `T_Hire_PartyPanel_V12` | Hire | 480×785 | 0.61 | - | - |
| `T_Hire_PartyPanel_V10` | Hire | 420×653 | 0.64 | 0.01~0.50 | 0.36~0.76 |
| `T_Hire_PartyFrame_V13` | Hire | 385×670 | 0.57 | - | - |

## 가로 띠 (제목판/스탯 스트립/행)  (21종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_wm_action_button_frame_normal` | WorldMap | 2055×567 | 3.62 | - | 0.06~0.15 |
| `T_wm_action_button_frame_pressed` | WorldMap | 1979×496 | 3.99 | - | 0.06~0.16 |
| `T_set_tab_frame` | Settings | 2000×469 | 4.26 | - | - |
| `T_MB_ArtifactTray_Frame` | Combat | 1841×468 | 3.93 | - | - |
| `T_hp_pill_frame` | Concept02 | 1780×483 | 3.69 | - | 0.04~0.15 |
| `T_menu_button_frame_normal` | Title | 1699×499 | 3.40 | 0.07~0.15 | 0.86~0.93 | - |
| `T_set_row_plate` | Settings | 2036×408 | 4.99 | - | 0.05~0.13 |
| `T_CombatHUD_UnitHpBar_Backplate` | UnitHpBar | 1958×370 | 5.29 | 0.03~0.97 | - |
| `T_CombatHUD_UnitHpBar_Backplate_FrameOnly` | UnitHpBar | 1958×370 | 5.29 | 0.03~0.97 | - |
| `T_dice_tray_frame` | Concept02 | 1702×397 | 4.29 | - | - |
| `T_Reward_RowFrame_Current` | RunFlow | 1536×384 | 4.00 | - | 0.09~0.20 | 0.74~0.85 |
| `T_room_name_banner_frame` | Concept02 | 1690×321 | 5.26 | - | - |
| `T_Hire_CarouselBase_V11` | Hire | 1745×308 | 5.67 | - | - |
| `T_turn_order_strip_frame` | Concept02 | 1774×275 | 6.45 | - | - |
| `T_set_slider_track` | Settings | 1727×277 | 6.24 | - | 0.06~0.16 |
| `T_Hire_PartyTray_V08` | Hire | 1426×320 | 4.46 | - | 0.01~0.12 | 0.86~0.96 |
| `T_Reward_SummaryPanel_V1` | Reward | 1536×256 | 6.00 | 0.00~0.99 | 0.03~0.12 | 0.16~0.84 | 0.85~0.94 |
| `T_MB_HireRowNormal` | Hire | 1024×288 | 3.56 | 0.01~0.96 | 0.07~0.90 |
| `T_MB_HireRowSelected` | Hire | 1024×288 | 3.56 | 0.01~0.98 | 0.07~0.95 |
| `T_MB_HireTitlePlate` | Hire | 1024×254 | 4.03 | 0.01~0.90 | 0.91~0.98 | 0.06~0.13 | 0.35~0.47 |
| `T_Hire_DetailNameplate_V08` | Hire | 960×252 | 3.81 | - | 0.00~0.09 |

## 버튼/행 판  (19종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_UI_Button_Blue_Frame` | Common | 1774×887 | 2.00 | 0.04~0.17 | 0.19~0.32 | 0.33~0.80 | 0.83~0.96 | - |
| `T_MB_Defeat_TitleBanner` | Defeat | 1862×845 | 2.20 | 0.04~0.18 | 0.20~0.81 | 0.85~0.97 | 0.30~0.58 |
| `T_RS_ExpTrack` | RewardSettlement | 1974×797 | 2.48 | - | - |
| `T_RS_TitlePlate` | RewardSettlement | 2149×732 | 2.94 | 0.03~0.10 | 0.91~0.98 | - |
| `T_RS_MercenaryRow` | RewardSettlement | 1923×817 | 2.35 | 0.02~0.98 | 0.24~0.76 |
| `T_MT_RowNormal` | MonsterTab | 1903×681 | 2.79 | 0.04~0.96 | 0.12~0.88 |
| `T_MT_RowSelected` | MonsterTab | 1903×681 | 2.79 | 0.04~0.96 | 0.12~0.89 |
| `T_MB_RoundBadge_Frame` | Combat | 1839×573 | 3.21 | - | - |
| `KK_HUD04_ap_number_plate` | HUD04 | 1689×584 | 2.89 | - | - |
| `T_MB_OptionsRail_Frame` | Combat | 1554×571 | 2.72 | - | - |
| `T_btn_frame_normal` | Concept02 | 1682×527 | 3.19 | - | - |
| `T_confirm_button_frame_normal` | ClassSelect | 1675×522 | 3.21 | 0.05~0.13 | 0.87~0.94 | 0.05~0.13 |
| `T_gold_pill_frame` | Concept02 | 1574×520 | 3.03 | 0.04~0.16 | 0.85~0.96 | 0.06~0.17 |
| `T_lv_pill_frame` | Concept02 | 1639×498 | 3.29 | 0.05~0.12 | 0.88~0.95 | - |
| `T_Reward_ResultPanel_V2` | Reward | 1446×553 | 2.62 | - | - |
| `T_version_plate` | Title | 1453×429 | 3.39 | - | - |
| `lv_pill_frame` | Concept02 | 1050×498 | 2.11 | 0.07~0.19 | 0.81~0.92 | - |
| `T_MB_HirePartyRowEmpty` | Hire | 1024×420 | 2.44 | 0.02~0.98 | 0.11~0.94 |
| `T_back_button_frame_normal` | ClassSelect | 1022×380 | 2.69 | - | 0.04~0.11 |

## 정사각 슬롯 (아이콘 홀더)  (11종)

| 에셋 | 폴더 | 크기 | 비율 | 세로 칸 (x) | 가로 칸 (y) |
|---|---|---|---|---|---|
| `T_Hire_Background_V08` | Hire | 2048×2048 | 1.00 | 0.11~0.20 | 0.29~0.77 | 0.00~0.19 | 0.60~0.76 |
| `T_StageMap_PopupBackground` | RunFlow | 2048×2048 | 1.00 | 0.13~0.86 | 0.06~0.82 |
| `T_MB_ArtifactSlot_Frame` | Combat | 1254×1254 | 1.00 | 0.12~0.23 | 0.76~0.91 | 0.08~0.28 | 0.64~0.86 |
| `T_MB_StatusSlot_Frame` | Combat | 1254×1254 | 1.00 | 0.12~0.88 | 0.11~0.88 |
| `T_MB_ActionButtonFrame` | Common | 1254×1254 | 1.00 | - | - |
| `T_MB_HireSkillButtonFrame` | Hire | 1254×1254 | 1.00 | - | 0.19~0.78 |
| `T_SkillCard_Frame_Combat` | Combat | 1217×1292 | 0.94 | - | - |
| `T_skill_slot_frame_normal` | Concept02 | 1215×1221 | 0.99 | - | - |
| `T_nav_button_frame_normal` | Concept02 | 1104×1113 | 0.99 | - | - |
| `T_Hire_PartyPanel_V11` | Hire | 953×974 | 0.98 | - | - |
| `T_Hire_Arrow_V11` | Hire | 565×605 | 0.93 | 0.03~0.15 | - |
