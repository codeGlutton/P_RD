# -*- coding: utf-8 -*-
"""시안4 전투 HUD 의 자리표. prepare_hud04.py 가 만든다.

손으로 고치지 마라. 시안이 바뀌면 다시 만든다 -- 손으로 고친 값은
다음에 만들 때 조용히 사라진다.

PLACE   판을 화면 어디에 놓나. 시안 1672 x 941 기준.
DETAIL  판 안의 글자와 그림 자리. **화면 기준**이다.

판 안 기준이 아니라 화면 기준으로 두는 이유: 판을 캔버스에 놓고 그 안에
다시 얹으면 두 좌표계를 오가야 하고, 그러다 한 번 어긋나면 어느 쪽이
틀렸는지 안 보인다. 전부 한 좌표계로 둔다.
"""

#: 판 이름 -> 텍스처 이름.
TEXTURE = {
    "action_bottom": "KK_HUD04_action_bottom",
    "action_left_lower": "KK_HUD04_action_left_lower",
    "action_left_upper": "KK_HUD04_action_left_upper",
    "action_right_lower": "KK_HUD04_action_right_lower",
    "action_right_upper": "KK_HUD04_action_right_upper",
    "action_top": "KK_HUD04_action_top",
    "bottom_right_button": "KK_HUD04_bottom_right_button",
    "bottom_status_center": "KK_HUD04_bottom_status_center",
    "bottom_status_left": "KK_HUD04_bottom_status_left",
    "bottom_status_right": "KK_HUD04_bottom_status_right",
    "top_center_turn_order": "KK_HUD04_top_center_turn_order",
    "top_left_parchment": "KK_HUD04_top_left_parchment",
    "top_right_parchment": "KK_HUD04_top_right_parchment",
    "upper_right_enemy_panel": "KK_HUD04_upper_right_enemy_panel",
}

#: 판을 화면 어디에 놓나. (x, y, w, h)
PLACE = {
    "action_bottom": (741, 666, 197, 217),
    "action_left_lower": (497, 542, 196, 213),
    "action_left_upper": (497, 303, 196, 218),
    "action_right_lower": (971, 542, 196, 213),
    "action_right_upper": (971, 303, 196, 218),
    "action_top": (741, 178, 197, 212),
    "bottom_right_button": (1318, 770, 345, 158),
    "bottom_status_center": (232, 761, 225, 174),
    "bottom_status_left": (10, 761, 225, 174),
    "bottom_status_right": (454, 760, 222, 175),
    "top_center_turn_order": (483, 12, 713, 158),
    "top_left_parchment": (15, 12, 309, 151),
    "top_right_parchment": (1271, 12, 392, 151),
    "upper_right_enemy_panel": (1273, 180, 390, 286),
}

#: 판 안의 자리. 판 이름 -> {요소 이름: (x, y, w, h)}
DETAIL = {
    "action_bottom": {
        "action_name": (808, 693, 49, 30),
        "action_icon": (794, 725, 84, 91),
        "cost_badge": (873, 786, 50, 51),
        "damage_text": (776, 796, 92, 30),
        "cooldown_text": (794, 824, 63, 29),
        "cooldown_overlay": (741, 666, 188, 210),
        "stance_text": (776, 796, 92, 30),
    },
    "action_left_lower": {
        "action_name": (564, 569, 49, 30),
        "action_icon": (550, 601, 84, 91),
        "cost_badge": (629, 662, 50, 51),
        "damage_text": (532, 672, 92, 30),
        "cooldown_text": (550, 700, 63, 29),
        "cooldown_overlay": (497, 542, 188, 210),
        "stance_text": (532, 672, 92, 30),
    },
    "action_left_upper": {
        "action_name": (564, 330, 49, 30),
        "action_icon": (550, 362, 84, 91),
        "cost_badge": (629, 423, 50, 51),
        "damage_text": (532, 433, 92, 30),
        "cooldown_text": (550, 461, 63, 29),
        "cooldown_overlay": (497, 303, 188, 210),
        "stance_text": (532, 433, 92, 30),
    },
    "action_right_lower": {
        "action_name": (1038, 569, 49, 30),
        "action_icon": (1024, 601, 84, 91),
        "cost_badge": (1103, 662, 50, 51),
        "damage_text": (1006, 672, 92, 30),
        "cooldown_text": (1024, 700, 63, 29),
        "cooldown_overlay": (971, 542, 188, 210),
        "stance_text": (1006, 672, 92, 30),
    },
    "action_right_upper": {
        "action_name": (1038, 330, 49, 30),
        "action_icon": (1024, 362, 84, 91),
        "cost_badge": (1103, 423, 50, 51),
        "damage_text": (1006, 433, 92, 30),
        "cooldown_text": (1024, 461, 63, 29),
        "cooldown_overlay": (971, 303, 188, 210),
        "stance_text": (1006, 433, 92, 30),
    },
    "action_top": {
        "action_name": (808, 205, 49, 30),
        "action_icon": (794, 237, 84, 91),
        "cost_badge": (873, 298, 50, 51),
        "damage_text": (776, 308, 92, 30),
        "cooldown_text": (794, 336, 63, 29),
        "cooldown_overlay": (741, 178, 188, 210),
        "stance_text": (776, 308, 92, 30),
    },
    "bottom_right_button": {
        "button_label": (1417, 822, 140, 49),
    },
    "bottom_status_center": {
        "party_portrait": (253, 786, 65, 80),
        "character_name": (323, 790, 46, 27),
        "hp_icon": (323, 814, 24, 24),
        "hp_value": (351, 814, 69, 25),
        "hp_bar": (350, 838, 79, 14),
        "ap_gems": (320, 872, 108, 31),
    },
    "bottom_status_left": {
        "party_portrait": (31, 786, 65, 80),
        "character_name": (101, 790, 46, 27),
        "hp_icon": (101, 814, 24, 24),
        "hp_value": (129, 814, 64, 25),
        "hp_bar": (128, 838, 79, 14),
        "status_icon": (101, 843, 25, 27),
        "status_text": (128, 847, 72, 24),
        "ap_gems": (98, 872, 108, 31),
        "selected_outline": (10, 761, 217, 166),
    },
    "bottom_status_right": {
        "party_portrait": (474, 785, 65, 80),
        "character_name": (544, 789, 61, 27),
        "hp_icon": (544, 813, 24, 24),
        "hp_value": (572, 813, 66, 25),
        "hp_bar": (571, 837, 79, 14),
        "ap_gems": (541, 871, 108, 31),
    },
    "top_center_turn_order": {
        "turn_portrait_01": (514, 40, 76, 74),
        "turn_portrait_02": (644, 40, 76, 74),
        "turn_portrait_03": (774, 40, 76, 74),
        "turn_portrait_04": (904, 40, 76, 74),
        "turn_portrait_05": (1034, 40, 76, 74),
        "selected_outline": (508, 34, 88, 86),
    },
    "top_left_parchment": {
        "round_label": (87, 60, 158, 44),
    },
    "top_right_parchment": {
        "objective_text": (1321, 60, 286, 44),
    },
    "upper_right_enemy_panel": {
        "enemy_portrait": (1306, 217, 113, 160),
        "enemy_name": (1430, 222, 69, 35),
        "hp_icon": (1430, 261, 39, 38),
        "hp_value": (1477, 266, 67, 30),
        "hp_bar": (1477, 292, 138, 17),
        "defense_icon": (1431, 312, 40, 42),
        "defense_text": (1476, 317, 75, 31),
        "damage_icon": (1430, 355, 43, 42),
        "damage_text": (1476, 362, 140, 31),
    },
}
