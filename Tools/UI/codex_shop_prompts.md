# 코덱스용 — Marchbound 상점 UI 시안 프롬프트 (4장 + 파츠)

상점 방(Shop Room)의 화면 시안을 뽑아줘. 로직은 이미 구현돼 있고 **UI만 없다**.
아래 영어 프롬프트를 그대로 쓰되, **먼저 "아트톤 기준" 파일들을 직접 열어 보고**
톤이 어긋나면 프롬프트를 다듬어도 된다. 목표는 "이 게임에 원래 있던 화면".

---

## 아트톤 기준 (생성 전 반드시 열어 볼 것)

참고 폴더: `D:\UnrealProjects\P_RD_develop_20260803\Tools\UI\mockups\assets\`

| 무엇 | 파일 | 여기서 볼 것 |
|---|---|---|
| 판(패널) 톤 | `T_MB_GenericDetailPanel.png` | 광낸 나무 테두리 + 황동 모서리판·리벳, 안쪽 짙은 목탄 슬레이트, 청록 문양 |
| 줄판(양피지) | `T_MB_HireRowNormal.png` / `T_MB_HireRowSelected.png` | 목록 한 줄의 양피지 톤·선택 상태 |
| 띠판 | `T_MB_HireStatsStrip.png` | 밝은 양피지 가로 띠 + 가는 금테 |
| 초상 틀 | `T_KitA_Portrait_Frame.png` | 정사각 나무+금 모서리 틀 |
| 소켓 | `T_Hire_DetailSkillSocket_V08.png` | 원형 스킬 소켓 |
| 아이콘 톤 | `T_SkillIcon_Whirlwind.png`, `T_Artifact_BloodChalice.png` | 채도 높은 손그림 아이콘, 금색 강조 |
| 단추 | `T_KitA_Button_Wide_Normal.png`, `T_Hire_ButtonOrange_V12.png` | 나무+금 단추 |
| 금화 | `T_gold_icon.png`, `T_Shop_GoldPlate_V01.png` | 재화 표기 |
| 이미 있는 상점 부품 | `T_Shop_TitlePlate_V01.png`, `T_Shop_ButtonBlue_V01.png`, `T_Shop_ButtonOrange_V01.png` | 상점 전용 톤 — **이것들을 우선 쓸 것** |

**공통 스타일 문구 (모든 프롬프트 앞에 붙일 것):**

```
Stylized casual fantasy game UI, mobile-game quality, clean vector-crisp shapes.
Polished warm wood frames with brass/gold corner plates and rivets, dark charcoal
slate inner panels, subtle teal filigree corner ornaments, light parchment strips
for rows. Cute chibi 3D-cartoon character portraits (big heads, dot eyes).
Vivid hand-painted icons with gold accents. Warm palette: dark brown #2a2016,
wood #6b5433, gold #f0c479, parchment #e8d7b4, teal accent #57c8d8.
Korean fantasy SRPG. No photorealism, no grimdark, no thin flat-design lines.
```

**공통 네거티브:** `photorealistic, realistic human faces, grimdark, thin minimal flat UI, blurry, watermark, lorem ipsum paragraphs`

**글자 처리:** 글자는 엔진이 얹는다(숫자·영문 Oswald Bold, 한글 LINE Seed KR Bold).
시안에는 **짧은 한글 견본**만 넣고 긴 문단은 흐린 잔줄로. 화면 크기는 **1920x1080**.

---

## 화면 흐름 (이 구조를 지켜서 그릴 것)

```
상점 로비 (1) ─┬─ 휴식 (2)
               ├─ 아티팩트 상점 (3)
               ├─ 스킬 상점 (4)
               └─ 용병 고용 → **기존 용병 선택 화면 재사용, 시안 불필요**
```

---

## 1. 상점 로비 — 1920x1080

들어서면 처음 보는 화면. 넷 중 하나를 고른다.

```
[공통 스타일 문구] Full-screen shop lobby for a fantasy SRPG, 1920x1080.
A cozy merchant tent/shop interior as the backdrop, dimmed. Centered wooden
panel with a title plate ("상점") at top and a gold counter at top-right
(coin icon + number). In the panel body, FOUR large square choice cards in a
row, each = ornate wood-and-gold frame + big hand-painted icon + a short
Korean label plate underneath: (1) 휴식 — a campfire/bedroll icon,
(2) 아티팩트 — a golden chalice/trinket icon, (3) 스킬 — a glowing skill rune
icon, (4) 용병 고용 — a chibi knight bust icon. One card shown hovered with a
gold glow. Bottom-right a wooden "나가기" button. Chunky, tappable, generous
spacing.
```

## 2. 휴식 화면 — 1920x1080

**휴식은 고르면 바로 끝나는 단순 화면.** 파티 회복량을 보여 주고 확인만 받는다.

```
[공통 스타일 문구] Full-screen "rest" screen for a fantasy SRPG, 1920x1080.
A warm campfire scene backdrop, dimmed. Centered wooden panel titled "휴식".
Inside: a row of THREE chibi mercenary portraits in square ornate frames
(knight, ranger, mage), each with a green HP bar underneath showing a partial
bar with a lighter green "heal preview" segment appended at its right end, and
a small "+30" text plate above it. Below the row, one parchment strip line for
the cost ("무료" or a coin icon + number). Bottom: a large gold "휴식하기"
button and a plain wood "돌아가기" button side by side. Calm, warm lighting.
```

## 3. 아티팩트 상점 — 1920x1080

**공용 물건이라 고르고 사기만 하면 된다.** 용병 선택 없음.

```
[공통 스타일 문구] Full-screen artifact shop screen for a fantasy SRPG,
1920x1080. Centered wooden panel titled "아티팩트 상점", gold counter at
top-right. LEFT (about 60%): a grid of 6 artifact offer cards (3 columns x 2
rows); each card = wood-and-gold frame + hand-painted artifact icon (chalice,
amulet, coin, crest, map, shield ornament) + a name plate + a price row with a
gold coin icon; one card highlighted as selected with a green glow; one card
shown as already-bought (darkened with a "판매됨" band). RIGHT (about 40%):
a detail column on dark slate — big icon in an ornate square slot, name plate,
a row of 5 small rarity gems (3 lit gold), a parchment "효과" strip, effect
text lines, and at the bottom a large gold "구매" button with the price and a
plain wood "돌아가기" button.
```

## 4. 스킬 상점 — 1920x1080

**핵심: 용병마다 스킬이 다르다.** 왼쪽에서 용병을 고르고, 그 용병의 스킬을
사거나 **교체**한다. 교체가 되는 화면이라는 것이 그림에서 읽혀야 한다.

```
[공통 스타일 문구] Full-screen skill shop screen for a fantasy SRPG,
1920x1080. Centered wooden panel titled "스킬 상점", gold counter top-right.
THREE columns.
LEFT (narrow): a vertical list of 3 owned mercenary rows on parchment strips —
each row = small chibi portrait + name plate + "Lv 1" badge; the first row is
selected with a cyan glowing border.
MIDDLE: "보유 스킬" heading, then the selected mercenary's SIX round skill
sockets in two rows of three, each holding a vivid hand-painted skill icon,
two sockets empty; one socket is highlighted with a red-gold "교체 대상"
marker ring.
RIGHT: "판매 중" heading, then a vertical list of 3 skill offer cards — each
card = round skill socket with icon + name plate + short stat strip (AP cost)
+ price row with coin icon; the middle card selected with a gold glow.
BOTTOM CENTER: a wide gold "교체하기" button with a two-way arrow glyph
between two small skill icons (the one being replaced and the new one), plus a
gold "구매" button and a plain wood "돌아가기" button. The swap arrow must be
clearly readable as "these two trade places".
```

---

## 5. 파츠 이미지 (낱장) — 위 화면에서 쓸 부품

**전 파츠 공통 규칙 (매우 중요):**
- **글자 절대 금지** — 글자는 엔진이 얹는다. 판·띠·단추 전부 빈 판으로.
- **아이콘도 금지** — 소켓·슬롯은 빈 구멍으로.
- 투명 배경 PNG(알파), 부품 하나당 한 장, 정면, 기울임·원근 없음.
- 늘려 쓰는 판/띠/단추는 **9-slice 가능하게**: 모서리 장식 고정, 가운데·변은 균일.
- 빛 방향·질감은 `T_MB_GenericDetailPanel.png` 과 같게.
- 파일명은 `T_MB_Shop_` 접두어 + 역할.

```
T_MB_Shop_ChoiceCard_Normal     로비의 큰 선택 카드 틀 (빈 판)
T_MB_Shop_ChoiceCard_Hover      위와 같되 금색 광 상태
T_MB_Shop_OfferCard_Normal      아티팩트/스킬 판매 카드 틀 (빈 구멍)
T_MB_Shop_OfferCard_Selected    위와 같되 금색 광 선택 상태
T_MB_Shop_OfferCard_SoldOut     위와 같되 어둡게 (팔린 것)
T_MB_Shop_PricePlate            가격 표기용 작은 양피지 판 (글자·동전 없이)
T_MB_Shop_SwapBadge             교체 표시용 양방향 화살 배지 (금색, 글자 없이)
T_MB_Shop_ReplaceRing           교체 대상 소켓에 두르는 붉은-금 테 (빈 고리)
T_MB_Shop_HealPreviewFill       HP바 회복 미리보기용 밝은 초록 채움 띠
```

각 파츠 프롬프트 뼈대:
```
[공통 스타일 문구] Single isolated UI part on a fully transparent background,
no text, no icons, no letters, front-facing, no perspective. {부품 설명}.
Same wood-brass-slate material language and lighting as the reference panel.
Uniform stretchable middle area, ornate details only at corners/ends.
```

---

## 코덱스에게 마무리 부탁

- 화면 시안 4장은 `sian_shop_01_로비.png` 처럼 **무슨 화면인지 파일명에** 적어서 저장.
- 파츠는 위 파일명 그대로, **투명 배경 확인** 후 저장.
- 톤 판정 기준: `T_MB_GenericDetailPanel.png` + `T_MB_HireRowNormal.png` 옆에 놓았을 때
  한 게임으로 보이는가. 어긋나면 그 둘을 참조 이미지로 걸고 다시.
- 네 화면은 **같은 프레임 문법**을 유지해야 한다 — 로비에서 들어간 화면들이니
  서로 딴 게임처럼 보이면 안 됨.
- 용병 고용 화면은 **만들지 말 것** (기존 용병 선택 화면을 재사용한다).
