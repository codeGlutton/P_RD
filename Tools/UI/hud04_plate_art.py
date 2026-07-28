# -*- coding: utf-8 -*-
"""갈아 끼운 판 그림. apply_zones.py 가 만든다.

판 이름 -> HUD04 폴더의 PNG 이름. 여기 없는 판은 시안에서
오려 낸 것을 그대로 쓴다.
"""

PLATE_ART = {
    # 목표 글자 대신 메뉴 넷을 놓는다. 판이 아니라 그림 한 장이라 늘리면
    # 나뭇결이 뭉갠다 -- plate() 에 맞추기를 시켜 비율을 지킨다.
    "top_right_parchment": "KK_HUD04_menu_bar",
}
