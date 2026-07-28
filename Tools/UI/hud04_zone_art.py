# -*- coding: utf-8 -*-
"""구역에 얹은 그림. apply_zones.py 가 만든다.

판 이름 -> 요소 -> {texture, fit}. texture 는 HUD04 폴더의 PNG 이름이고,
import_hud04.py 가 그 폴더를 통째로 넣는다.
"""

ZONE_ART = {
    "action_top": {
        "cooldown_badge": {"texture": "KK_HUD04_zone_cooldown_badge", "fit": "contain"},
        "cost_badge": {"texture": "KK_HUD04_zone_cost_badge", "fit": "contain"},
    },
    "bottom_status_left": {
        "hp_icon": {"texture": "KK_HUD04_zone_hp_icon", "fit": "contain"},
    },
    "bottom_status_right": {
        "hp_icon": {"texture": "KK_HUD04_zone_hp_icon", "fit": "contain"},
    },
}
