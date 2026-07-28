# -*- coding: utf-8 -*-
"""구역 조정 쪽에서 손으로 맞춘 자리. apply_zones.py 가 만든다.

손으로 고쳐도 된다. 다만 쪽에서 다시 내려받아 넣으면 덮인다.

값은 **판 안** 자리다. 판 왼쪽 위가 원점이고 (x, y, w, h) 다.
화면 자리로 옮기는 것은 prepare_hud04.py 가 한다.
"""

#: 판 이름 -> 요소 -> (x, y, w, h). 시안을 잰 값을 이것으로 덮는다.
TUNING = {
    "action_bottom": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "action_left_lower": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "action_left_upper": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "action_right_lower": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "action_right_upper": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "action_top": {
        "action_icon": (54, 56, 89, 89),
        "action_name": (28, 27, 142, 27),
        "cooldown_overlay": (9, 7, 180, 196),
        "cooldown_text": (28, 27, 142, 157),
        "cost_badge": (134, 75, 49, 51),
        "damage_text": (28, 146, 142, 19),
        "stance_text": (28, 165, 142, 19),
    },
    "bottom_right_button": {
        "button_label": (41, 41, 264, 74),
    },
    "bottom_status_center": {
        "ap_gems": (88, 111, 108, 31),
        "character_name": (91, 29, 46, 27),
        "hp_bar": (118, 77, 79, 14),
        "hp_icon": (91, 53, 24, 24),
        "hp_value": (119, 53, 69, 25),
        "party_portrait": (21, 25, 65, 80),
    },
    "bottom_status_left": {
        "ap_gems": (88, 111, 108, 31),
        "character_name": (91, 29, 46, 27),
        "hp_bar": (118, 77, 79, 14),
        "hp_icon": (91, 53, 24, 24),
        "hp_value": (119, 53, 64, 25),
        "party_portrait": (21, 25, 65, 80),
        "selected_outline": (0, 0, 217, 166),
        "status_icon": (91, 82, 25, 27),
        "status_text": (118, 86, 72, 24),
    },
    "bottom_status_right": {
        "ap_gems": (29, 114, 165, 31),
        "character_name": (96, 29, 61, 27),
        "hp_bar": (97, 87, 94, 14),
        "hp_icon": (97, 59, 24, 24),
        "hp_value": (125, 59, 66, 25),
        "party_portrait": (27, 29, 65, 80),
    },
    "top_center_turn_order": {
        "selected_outline": (65, 32, 96, 96),
        "turn_portrait_01": (70, 37, 85, 85),
        "turn_portrait_02": (193, 37, 85, 85),
        "turn_portrait_03": (315, 37, 85, 85),
        "turn_portrait_04": (438, 37, 85, 85),
        "turn_portrait_05": (560, 37, 85, 85),
    },
    "top_left_parchment": {
        "round_label": (38, 37, 233, 76),
    },
    "top_right_parchment": {
        "objective_text": (38, 37, 317, 76),
    },
    "upper_right_enemy_panel": {
        "damage_icon": (157, 191, 43, 42),
        "damage_text": (203, 191, 140, 31),
        "defense_icon": (157, 138, 40, 42),
        "defense_text": (201, 138, 75, 31),
        "enemy_name": (157, 40, 193, 35),
        "enemy_portrait": (40, 40, 113, 160),
        "hp_bar": (201, 116, 138, 17),
        "hp_icon": (157, 80, 39, 38),
        "hp_value": (201, 81, 67, 30),
    },
}
