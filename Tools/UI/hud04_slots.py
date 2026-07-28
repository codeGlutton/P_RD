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
    "action_bottom": (741, 666, 197, 212),
    "action_left_lower": (497, 542, 197, 212),
    "action_left_upper": (497, 303, 197, 212),
    "action_right_lower": (971, 542, 197, 212),
    "action_right_upper": (971, 303, 197, 212),
    "action_top": (741, 178, 197, 212),
    "bottom_right_button": (1318, 770, 345, 158),
    "bottom_status_center": (232, 761, 225, 174),
    "bottom_status_left": (10, 761, 225, 174),
    "bottom_status_right": (454, 760, 225, 174),
    "top_center_turn_order": (426, 12, 819, 158),
    "top_left_parchment": (15, 12, 309, 151),
    "top_right_parchment": (1271, 12, 392, 121),
    "upper_right_enemy_panel": (1273, 180, 390, 286),
}

#: 판 안의 자리. 판 이름 -> {요소 이름: (x, y, w, h)}
DETAIL = {
    "action_bottom": {
        "action_icon": (791, 729, 97, 100),
        "action_name": (769, 701, 142, 27),
        "cooldown_badge": (897, 776, 49, 51),
        "cooldown_overlay": (750, 673, 180, 196),
        "cooldown_text": (769, 693, 142, 157),
        "cost_badge": (897, 723, 49, 51),
        "damage_text": (769, 829, 142, 19),
        "stance_text": (786, 799, 92, 29),
    },
    "action_left_lower": {
        "action_icon": (547, 605, 97, 100),
        "action_name": (525, 577, 142, 27),
        "cooldown_badge": (653, 652, 49, 51),
        "cooldown_overlay": (506, 549, 180, 196),
        "cooldown_text": (525, 569, 142, 157),
        "cost_badge": (653, 599, 49, 51),
        "damage_text": (525, 705, 142, 19),
        "stance_text": (542, 675, 92, 29),
    },
    "action_left_upper": {
        "action_icon": (547, 366, 97, 100),
        "action_name": (525, 338, 142, 27),
        "cooldown_badge": (653, 413, 49, 51),
        "cooldown_overlay": (506, 310, 180, 196),
        "cooldown_text": (525, 330, 142, 157),
        "cost_badge": (653, 360, 49, 51),
        "damage_text": (525, 466, 142, 19),
        "stance_text": (542, 436, 92, 29),
    },
    "action_right_lower": {
        "action_icon": (1021, 605, 97, 100),
        "action_name": (999, 577, 142, 27),
        "cooldown_badge": (1127, 652, 49, 51),
        "cooldown_overlay": (980, 549, 180, 196),
        "cooldown_text": (999, 569, 142, 157),
        "cost_badge": (1127, 599, 49, 51),
        "damage_text": (999, 705, 142, 19),
        "stance_text": (1016, 675, 92, 29),
    },
    "action_right_upper": {
        "action_icon": (1021, 366, 97, 100),
        "action_name": (999, 338, 142, 27),
        "cooldown_badge": (1127, 413, 49, 51),
        "cooldown_overlay": (980, 310, 180, 196),
        "cooldown_text": (999, 330, 142, 157),
        "cost_badge": (1127, 360, 49, 51),
        "damage_text": (999, 466, 142, 19),
        "selected_outline": (971, 303, 188, 210),
        "stance_text": (1016, 436, 92, 29),
    },
    "action_top": {
        "action_icon": (797, 241, 85, 85),
        "action_name": (769, 213, 142, 27),
        "cooldown_badge": (897, 288, 49, 51),
        "cooldown_overlay": (750, 185, 180, 196),
        "cost_badge": (897, 235, 49, 51),
        "damage_text": (768, 328, 142, 27),
    },
    "bottom_right_button": {
        "button_label": (1359, 811, 264, 74),
    },
    "bottom_status_center": {
        "ap_number": (333, 821, 93, 31),
        "ap_number_txt": (346, 827, 68, 19),
        "character_name": (333, 792, 93, 26),
        "hp_bar": (333, 885, 93, 16),
        "hp_icon": (333, 855, 26, 26),
        "hp_value": (363, 855, 63, 26),
        "party_portrait": (264, 792, 66, 109),
        "status_icon": (283, 735, 36, 41),
        "status_icon_1": (327, 735, 36, 41),
        "status_icon_2": (371, 735, 36, 41),
        "status_icon_img": (289, 740, 24, 24),
        "status_icon_img_1": (333, 740, 24, 24),
        "status_icon_img_2": (377, 740, 24, 24),
    },
    "bottom_status_left": {
        "ap_number": (111, 821, 93, 31),
        "ap_number_txt": (124, 827, 68, 19),
        "character_name": (111, 792, 93, 26),
        "hp_bar": (111, 885, 93, 16),
        "hp_icon": (111, 855, 26, 26),
        "hp_value": (141, 855, 63, 26),
        "party_portrait": (42, 792, 66, 109),
        "status_icon": (61, 735, 36, 41),
        "status_icon_1": (105, 735, 36, 41),
        "status_icon_2": (149, 735, 36, 41),
        "status_icon_img": (67, 740, 24, 24),
        "status_icon_img_1": (111, 740, 24, 24),
        "status_icon_img_2": (155, 740, 24, 24),
    },
    "bottom_status_right": {
        "ap_number": (555, 820, 93, 31),
        "ap_number_txt": (568, 826, 68, 19),
        "character_name": (555, 791, 93, 26),
        "hp_bar": (555, 884, 93, 16),
        "hp_icon": (555, 854, 26, 26),
        "hp_value": (585, 854, 63, 26),
        "party_portrait": (486, 791, 66, 109),
        "status_icon": (505, 734, 36, 41),
        "status_icon_1": (549, 734, 36, 41),
        "status_icon_2": (593, 734, 36, 41),
        "status_icon_img": (511, 739, 24, 24),
        "status_icon_img_1": (555, 739, 24, 24),
        "status_icon_img_2": (599, 739, 24, 24),
    },
    "top_center_turn_order": {
        "end_left": (450, 64, 50, 55),
        "end_right": (1171, 64, 50, 55),
        "selected_outline": (491, 44, 96, 96),
        "selected_outline_01": (518, 44, 95, 95),
        "selected_outline_02": (626, 44, 95, 95),
        "selected_outline_03": (734, 44, 95, 95),
        "selected_outline_04": (842, 44, 95, 95),
        "selected_outline_05": (950, 44, 95, 95),
        "selected_outline_06": (1058, 44, 95, 95),
        "turn_portrait_01": (523, 49, 85, 85),
        "turn_portrait_02": (631, 49, 85, 85),
        "turn_portrait_03": (739, 49, 85, 85),
        "turn_portrait_04": (847, 49, 85, 85),
        "turn_portrait_05": (955, 49, 85, 85),
        "turn_portrait_06": (1063, 49, 85, 85),
    },
    "top_left_parchment": {
        "round_label": (53, 49, 233, 76),
    },
    "top_right_parchment": {
        "menu_bag": (2120, 95, 346, 351),
        "menu_map": (1378, 97, 348, 349),
        "menu_settings": (2490, 97, 347, 351),
        "menu_skill": (1751, 98, 344, 348),
        "objective_text": (1309, 49, 317, 76),
    },
    "upper_right_enemy_panel": {
        "damage_icon": (1430, 371, 43, 42),
        "damage_text": (1476, 371, 140, 31),
        "defense_icon": (1430, 318, 40, 42),
        "defense_text": (1474, 318, 75, 31),
        "enemy_name": (1430, 220, 193, 35),
        "enemy_portrait": (1313, 220, 113, 160),
        "hp_bar": (1474, 296, 138, 17),
        "hp_icon": (1430, 260, 39, 38),
        "hp_value": (1474, 261, 67, 30),
    },
}
