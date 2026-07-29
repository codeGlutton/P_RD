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
    "bottom_center_ap_bar": "KK_HUD04_ap_bar_steel",
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
    "bottom_center_ap_bar": (486, 800, 700, 129),
    "bottom_right_button": (1318, 770, 345, 158),
    "bottom_status_center": (118, 712, 96, 100),
    "bottom_status_left": (14, 712, 96, 100),
    "bottom_status_right": (222, 712, 96, 100),
    "top_center_turn_order": (426, 12, 819, 158),
    "top_left_parchment": (15, 12, 309, 151),
    "top_right_parchment": (1271, 12, 392, 121),
    "upper_right_enemy_panel": (1273, 180, 390, 286),
}

#: 판 안의 자리. 판 이름 -> {요소 이름: (x, y, w, h)}
DETAIL = {
    "action_bottom": {
        "action_icon": (797, 729, 85, 85),
        "action_name": (769, 701, 142, 27),
        "cooldown_badge": (897, 776, 49, 51),
        "cooldown_overlay": (750, 673, 180, 196),
        "cost_badge": (897, 723, 49, 51),
        "damage_text": (768, 816, 142, 27),
    },
    "action_left_lower": {
        "action_icon": (553, 605, 85, 85),
        "action_name": (525, 577, 142, 27),
        "cooldown_badge": (653, 652, 49, 51),
        "cooldown_overlay": (506, 549, 180, 196),
        "cost_badge": (653, 599, 49, 51),
        "damage_text": (524, 692, 142, 27),
    },
    "action_left_upper": {
        "action_icon": (553, 366, 85, 85),
        "action_name": (525, 338, 142, 27),
        "cooldown_badge": (653, 413, 49, 51),
        "cooldown_overlay": (506, 310, 180, 196),
        "cost_badge": (653, 360, 49, 51),
        "damage_text": (524, 453, 142, 27),
    },
    "action_right_lower": {
        "action_icon": (1027, 605, 85, 85),
        "action_name": (999, 577, 142, 27),
        "cooldown_badge": (1127, 652, 49, 51),
        "cooldown_overlay": (980, 549, 180, 196),
        "cost_badge": (1127, 599, 49, 51),
        "damage_text": (998, 692, 142, 27),
    },
    "action_right_upper": {
        "action_icon": (1027, 366, 85, 85),
        "action_name": (999, 338, 142, 27),
        "cooldown_badge": (1127, 413, 49, 51),
        "cooldown_overlay": (980, 310, 180, 196),
        "cost_badge": (1127, 360, 49, 51),
        "damage_text": (998, 453, 142, 27),
    },
    "action_top": {
        "action_icon": (797, 241, 85, 85),
        "action_name": (769, 213, 142, 27),
        "cooldown_badge": (897, 288, 49, 51),
        "cooldown_overlay": (750, 185, 180, 196),
        "cost_badge": (897, 235, 49, 51),
        "damage_text": (768, 328, 142, 27),
    },
    "bottom_center_ap_bar": {
        "ap_number": (510, 840, 111, 50),
        "ap_pip_01": (628, 840, 44, 50),
        "ap_pip_02": (680, 840, 44, 50),
        "ap_pip_03": (732, 840, 44, 50),
        "ap_pip_04": (784, 840, 44, 50),
        "ap_pip_05": (836, 840, 44, 50),
        "ap_pip_06": (888, 840, 44, 50),
        "ap_pip_07": (940, 840, 44, 50),
        "ap_pip_08": (992, 840, 44, 50),
        "ap_pip_09": (1044, 840, 44, 50),
        "ap_pip_10": (1096, 840, 44, 50),
    },
    "bottom_right_button": {
        "button_label": (1359, 811, 264, 74),
    },
    "bottom_status_center": {
        "ap_number": (128, 900, 76, 24),
        "ap_number_txt": (128, 902, 76, 20),
        "character_name": (128, 844, 76, 20),
        "hp_bar": (128, 870, 76, 24),
        "hp_icon": (116, 870, 22, 22),
        "hp_value": (128, 872, 76, 20),
        "party_portrait": (132, 754, 68, 84),
        "status_icon": (122, 718, 27, 30),
        "status_icon_1": (152, 718, 27, 30),
        "status_icon_2": (182, 718, 27, 30),
        "status_icon_img": (127, 722, 18, 18),
        "status_icon_img_1": (157, 722, 18, 18),
        "status_icon_img_2": (187, 722, 18, 18),
    },
    "bottom_status_left": {
        "ap_number": (24, 900, 76, 24),
        "ap_number_txt": (24, 902, 76, 20),
        "character_name": (24, 844, 76, 20),
        "hp_bar": (24, 870, 76, 24),
        "hp_icon": (12, 870, 22, 22),
        "hp_value": (24, 872, 76, 20),
        "party_portrait": (28, 754, 68, 84),
        "status_icon": (18, 718, 27, 30),
        "status_icon_1": (48, 718, 27, 30),
        "status_icon_2": (78, 718, 27, 30),
        "status_icon_img": (23, 722, 18, 18),
        "status_icon_img_1": (53, 722, 18, 18),
        "status_icon_img_2": (83, 722, 18, 18),
    },
    "bottom_status_right": {
        "ap_number": (232, 900, 76, 24),
        "ap_number_txt": (232, 902, 76, 20),
        "character_name": (232, 844, 76, 20),
        "hp_bar": (232, 870, 76, 24),
        "hp_icon": (220, 870, 22, 22),
        "hp_value": (232, 872, 76, 20),
        "party_portrait": (236, 754, 68, 84),
        "status_icon": (226, 718, 27, 30),
        "status_icon_1": (256, 718, 27, 30),
        "status_icon_2": (286, 718, 27, 30),
        "status_icon_img": (231, 722, 18, 18),
        "status_icon_img_1": (261, 722, 18, 18),
        "status_icon_img_2": (291, 722, 18, 18),
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
        "menu_bag": (1470, 31, 81, 82),
        "menu_map": (1296, 32, 82, 82),
        "menu_settings": (1557, 32, 81, 82),
        "menu_skill": (1384, 32, 81, 82),
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
