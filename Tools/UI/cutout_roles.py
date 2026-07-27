# -*- coding: utf-8 -*-
"""조각이 무엇인지 적어 둔 표. 시안에 번호를 그려 놓고 눈으로 보고 적었다.

## 왜 손으로 적었나

색과 크기로 판정하는 기계를 먼저 만들었다. 스무 장이 서로 다르게 생겨서,
한 시안을 맞추면 다른 시안이 틀어졌다 -- 1안은 아군판이 적 패널로, 3안은
턴순서가 턴종료로, 4안은 아래가 전부 아군으로 잡혔다. 규칙을 더 얹을수록
어디가 맞고 어디가 틀렸는지 알 수 없어졌다.

자리는 기계가 정확히 찾았다(match_cutouts). 남은 것은 이름뿐이고 이름은
보면 안다. annotate_places 로 번호를 그려 스무 장을 한 장씩 보고 적었다.
조각 이백 장에 표 이백 줄이면 손으로 적는 편이 빠르고 무엇보다 확실하다.

## 표 읽는 법

번호는 cutout_places.json 의 순서(위에서 아래, 왼쪽에서 오른쪽)다.

  round      라운드 표시        objective  목표 현판
  turn       턴 순서            party      아군 상태
  skill      스킬 카드          enemy      적 정보
  endturn    턴 종료 버튼
  band       여러 역할이 한 조각에 든 것. 쪼개지 않고 통째로 놓는다
  skip       배경에 잘못 붙은 것. 버린다

## 덮이지 않은 곳

조각이 없어 비는 자리가 있다(16안은 적 정보 하나뿐, 18안은 스킬과 적과
버튼이 통째로 없다). MISSING 에 적어 두었다 -- 배치안을 만들 때 그 구역은
빈 채로 두고, 채우려면 그 부분을 더 오려야 한다.
"""

#: 시안 번호 -> {조각 번호: 역할}
ROLES = {
    "01": {1: "turn", 2: "round", 3: "objective", 4: "party", 5: "skill",
           6: "enemy", 7: "endturn"},
    "02": {1: "turn", 2: "round", 3: "objective", 4: "party", 5: "enemy",
           6: "party", 7: "party", 8: "skill", 9: "skill", 10: "skill",
           11: "skill", 12: "skill", 13: "skill", 14: "endturn"},
    "03": {1: "turn", 2: "round", 3: "objective", 4: "party", 5: "enemy",
           6: "party", 7: "skill", 8: "skill", 9: "skill", 10: "skill",
           11: "skill", 12: "endturn"},
    "04": {1: "turn", 2: "round", 3: "objective", 4: "enemy", 5: "skill",
           6: "skill", 7: "skill", 8: "skill", 9: "skill", 10: "party",
           11: "party", 12: "party", 13: "endturn"},
    "05": {1: "turn", 2: "objective", 3: "round", 4: "enemy", 5: "band"},
    "06": {1: "round", 2: "objective", 3: "turn", 4: "turn", 5: "turn",
           6: "turn", 7: "turn", 8: "party", 9: "enemy", 10: "party",
           11: "party", 12: "skill", 13: "skill", 14: "skill", 15: "skill",
           16: "skill", 17: "skill", 18: "endturn"},
    "07": {1: "turn", 2: "round", 3: "party", 4: "enemy", 5: "skill",
           6: "skill", 7: "skill", 8: "skill", 9: "party", 10: "endturn"},
    "08": {1: "round", 2: "turn", 3: "turn", 4: "turn", 5: "turn",
           6: "enemy", 7: "party", 8: "skill", 9: "skill", 10: "skill",
           11: "skill", 12: "skill", 13: "skill", 14: "endturn"},
    "09": {1: "band", 2: "band"},
    "10": {1: "round", 2: "turn", 3: "turn", 4: "turn", 5: "turn",
           6: "objective", 7: "skip", 8: "party", 9: "enemy", 10: "skill",
           11: "skill", 12: "skill", 13: "skill", 14: "skill", 15: "skill",
           16: "endturn", 17: "skip"},
    "11": {1: "round", 2: "objective", 3: "turn", 4: "skip", 5: "endturn",
           6: "party", 7: "skill", 8: "skill", 9: "party", 10: "skill",
           11: "party", 12: "skill", 13: "skip"},
    "12": {1: "round", 2: "objective", 3: "turn", 4: "turn", 5: "enemy",
           6: "turn", 7: "skill", 8: "skill", 9: "skill", 10: "skill",
           11: "skill", 12: "skill", 13: "endturn"},
    "13": {1: "turn", 2: "objective", 3: "round", 4: "skill", 5: "enemy",
           6: "skill", 7: "skill", 8: "skill", 9: "skill", 10: "skill",
           11: "party", 12: "party", 13: "party", 14: "endturn"},
    "14": {1: "round", 2: "turn", 3: "objective", 4: "skill", 5: "enemy",
           6: "party", 7: "endturn"},
    "15": {1: "turn", 2: "round", 3: "objective", 4: "band"},
    "16": {1: "enemy"},
    "17": {1: "round", 2: "turn", 3: "objective", 4: "band", 5: "party",
           6: "party", 7: "party"},
    "18": {1: "round", 2: "objective", 3: "turn", 4: "party", 5: "party",
           6: "party"},
    "19": {1: "band", 2: "party", 3: "enemy", 4: "party", 5: "party",
           6: "turn", 7: "objective"},
    "20": {1: "band", 2: "enemy", 3: "party", 4: "party", 5: "enemy",
           6: "party", 7: "skill", 8: "skill", 9: "endturn"},
}

#: 조각이 없어 비는 구역. 배치안에서 이 구역은 비운다.
MISSING = {
    "07": ["party 한 줄(궁수)"],
    "11": ["enemy"],
    "16": ["round", "objective", "turn", "party", "skill", "endturn"],
    "18": ["skill", "enemy", "endturn"],
    "20": ["skill 네 장"],
}

#: band 안에 무엇이 들었는지. 통째로 놓되 구멍에서 내용 자리를 뽑는다.
BAND_CONTENTS = {
    ("05", 5): ["party", "skill", "endturn"],
    ("09", 1): ["round", "party", "turn", "enemy", "objective"],
    ("09", 2): ["skill", "endturn"],
    ("15", 4): ["party", "skill", "enemy", "endturn"],
    ("17", 4): ["enemy", "skill", "endturn"],
    ("19", 1): ["round", "skill", "endturn"],
    ("20", 1): ["round", "objective"],
}
