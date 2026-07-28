# -*- coding: utf-8 -*-
"""구역의 층. apply_zones.py 가 만든다.

판 이름 -> 요소 -> 층. 여기 없는 것은 굽는 쪽이 정한 대로 간다
(판 0, 내용 10, 글자 15, 표시 40).
"""

Z_ORDER = {
    "action_bottom": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
    "action_left_lower": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
    "action_left_upper": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
    "action_right_lower": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
    "action_right_upper": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
    "action_top": {
        "cooldown_badge": 50,
        "cost_badge": 50,
    },
}
