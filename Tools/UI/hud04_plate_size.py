# -*- coding: utf-8 -*-
"""판의 자리와 크기. apply_zones.py 가 쪽에서 받아 적는다.

손으로 고쳐도 된다. 쪽에서 다시 내려받아 넣으면 그쪽 값이 이긴다.

값이 둘이면 크기만, 넷이면 (x, y, w, h) 로 자리까지 갈아 끼운다.
넓히면서 자리를 안 주면 오른쪽으로만 자라 옆 판을 밀고 들어간다.

**hud04_tuning.py 에 두지 마라.** 그 파일은 이 스크립트가 통째로
다시 쓰므로, 거기 적어 두면 다음 내려받기에 조용히 사라진다.
"""

PLATE_SIZE = {
    "bottom_center_ap_bar": (326, 852, 355, 60),
    "bottom_status_center": (118, 712, 96, 200),
    "bottom_status_left": (14, 712, 96, 200),
    "bottom_status_right": (222, 712, 96, 200),
    "top_center_turn_order": (426, 12, 819, 158),
    "top_right_parchment": (392, 121),
}
