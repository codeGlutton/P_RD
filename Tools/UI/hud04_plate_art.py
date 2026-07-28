# -*- coding: utf-8 -*-
"""갈아 끼운 판 그림. apply_zones.py 가 만든다.

판 이름 -> HUD04 폴더의 PNG 이름. 여기 없는 판은 시안에서
오려 낸 것을 그대로 쓴다.
"""

PLATE_ART = {
    # 목표 글자 대신 메뉴 넷을 놓는다. 판이 아니라 그림 한 장이라 늘리면
    # 나뭇결이 뭉갠다 -- plate() 에 맞추기를 시켜 비율을 지킨다.
    "top_right_parchment": "KK_HUD04_menu_bar",
    # 여섯 칸 + 양끝 넘김칸. **이름을 달리 둔 까닭이 있다** -- 시안 것과 같은
    # 이름으로 두면 prepare_hud04.py 의 collect_plates() 가 시안 원본을 그 위에
    # 덮어쓴다. 실제로 덮였고, 편집기에 옛 판이 그대로 떴다.
    "top_center_turn_order": "KK_HUD04_turn_order_v2",
}
