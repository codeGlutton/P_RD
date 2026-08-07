# 코덱스용 — Marchbound UI 시안·파츠 이미지 생성 프롬프트

너(코덱스)에게 부탁: **전체 시안 3장**(1 용병탭 · 2 몬스터탭 · 7 설정판)과
**파츠 낱장**(5-1 소품 아이콘, 8 파츠 목록)을 이미지 생성으로 뽑아줘.
3·4·5·6번(요약판·상세판)은 시안이 이미 확정되어 생성이 필요 없다 — 참고용으로만 남겨 둠.
각 항목의 영어 프롬프트를 그대로 쓰되, **먼저 "아트톤 기준" 의 참고 파일들을 직접 열어 보고**
톤이 어긋나면 프롬프트를 네가 다듬어도 된다. 목표는 "이 게임에 원래 있던 화면처럼 보이는 것".

---

## 아트톤 기준 (모든 화면 공통 — 생성 전에 꼭 열어 볼 것)

참고 폴더: `D:\UnrealProjects\P_RD_develop_20260803\Tools\UI\mockups\assets\`

| 무엇 | 파일 | 여기서 볼 것 |
|---|---|---|
| 판(패널) 톤 | `T_MB_GenericDetailPanel.png` | 광낸 나무 테두리 + 황동/금 모서리판과 리벳, 안쪽은 짙은 목탄색 슬레이트, 모서리에 청록(teal) 문양 |
| 띠판 | `T_MB_HireStatsStrip.png` | 밝은 양피지(parchment) 가로 띠 + 가는 금테 |
| 초상 톤 | `T_MB_HireIcon_Knight.png` | 치비 3D 카툰(모바일 캐주얼, 큰 머리·검은 점 눈) — 실사 금지 |
| 적 초상 톤 | `KK_Face_Enemy_Eagle_HeadV2.png` | 적도 같은 치비 톤 |
| 아이콘 톤 | `T_SkillIcon_Whirlwind.png`, `T_Artifact_BloodChalice.png` | 채도 높은 손그림 게임 아이콘, 금색 강조, 보석은 각진 면 |
| 틀(슬롯) | `T_MB_ArtifactSlot_Frame.png`, `T_MB_StatusSlot_Frame.png` | 네모 슬롯 틀 규격 |
| HP바 | `T_CombatHUD_UnitHpBar_Backplate_FrameOnly.png` + `T_CombatHUD_UnitHpBar_Fill_Green.png`/`_Red.png` | 바 틀·채움 모양 |

**공통 스타일 문구 (모든 프롬프트 앞에 붙일 것):**

```
Stylized casual fantasy game UI, mobile-game quality, clean vector-crisp shapes.
Polished warm wood frames with brass/gold corner plates and rivets, dark charcoal
slate inner panels, subtle teal filigree corner ornaments, light parchment strips
for stat rows. Cute chibi 3D-cartoon character portraits (big heads, dot eyes,
Clash-style). Vivid hand-painted icons with gold accents. Warm palette:
dark brown #2a2016, wood #6b5433, gold #f0c479, parchment #e8d7b4, teal accent
#57c8d8. Ally elements green, enemy elements red. Korean fantasy SRPG.
No photorealism, no gritty dark-souls tone, no thin flat-design lines.
```

**공통 네거티브:** `photorealistic, realistic human faces, grimdark, horror, thin minimal flat UI, blurry, watermark, lorem ipsum paragraphs`

**글자 처리:** 실제 글자는 엔진이 얹는다(숫자·영문 Oswald Bold, 한글 LINE Seed KR Bold).
시안에는 자리만 보이면 되니 **짧은 한글/숫자 견본**(예: "고용", "출진", "100/100")으로 넣고,
긴 문단은 흐린 잔글 줄로 처리하라고 프롬프트에 이미 적어 뒀다.

**크기:** 전체 화면은 1920x1080(16:9), 판 하나짜리는 각 항목에 적은 비율로.

---

## 1. 용병탭 (보유 용병 상세) — 1920x1080

**주의: 고용 화면이 아니다.** 이미 데리고 있는 용병을 열람하는 상세탭이다 —
고용 비용·재화·고용 단추가 나오면 틀린 것.

참고: 위 공통 + `T_KitA_Portrait_Frame.png`(초상 틀), `T_MB_HireNamePlate.png`(이름띠),
`T_Hire_DetailSkillSocket_V08.png`(스킬 소켓), `T_MB_ArtifactSlot_Frame.png`(장비 슬롯),
`T_Hire_ButtonOrange_V12.png`(단추 톤)

```
[공통 스타일 문구] Full-screen "my mercenaries" roster detail tab for a fantasy
SRPG, 1920x1080. This is a barracks / unit-management screen, NOT a shop: no
prices, no coins, no hire button. Top header band with a title plate ("용병").
Left column (about 1/4 width): vertical list of 5-6 owned mercenary cards, each
card = small chibi portrait in a wood-and-gold slot frame + name strip + small
level badge ("Lv 5"); one card highlighted as selected with a green glow.
Right side (about 3/4): large detail panel — big chibi knight portrait in an
ornate square frame on the left, name plate with a class glyph beside it, a thin
gold EXP progress bar under the name, parchment stat strips for HP / AP / speed
with small icons, a row of 6 round skill sockets with vivid hand-painted skill
icons (2 sockets empty), and below a labelled row of 3 square artifact/equipment
slots (1 filled with a golden trinket icon, 2 empty). Bottom right one gold
"파티 편성" button. Wooden panel frames with brass corners on dark slate.
```

## 2. 몬스터탭 (도감) — 1920x1080

참고: 위 공통 + `KK_Face_Enemy_Eagle_HeadV2.png` 톤의 적 초상들

```
[공통 스타일 문구] Full-screen monster codex tab for a fantasy SRPG, 1920x1080.
Top header band with title plate ("몬스터"). Left half: grid of monster entries
(4 columns x 3 rows), each a square slot frame with a cute-but-menacing chibi
monster portrait (skeleton warrior, giant eagle, golem, slime...) and a small
name strip; undiscovered entries shown as darkened silhouettes with a "?".
Selected entry has a red glow. Right half: detail panel — large chibi monster
portrait, name plate, parchment stat strips (HP / AP / speed), a row of trait
or skill icons in square sockets, and below a short flavor-text area rendered
as faint placeholder lines on dark slate. Enemy accents in red.
```

## 3. 용병 요약판 — **생성 불필요.** 시안(sian_03) 확정, 기존 에셋으로 조립함.

## (참고용 원본) 용병 요약판 (전투 중 아군 요약) — 1200x860 (판 하나만, 16:11.5)

참고: 위 공통 + 지금 편집기 기본 배치(`Tools/UI/mockups/summary_design.html` 열어 보면
아군 판이 그 구성): 초상(왼 위) / 이름(가운데) / HP바 / AP 칸 · 속도 칸 / 상태 아이콘 3칸

```
[공통 스타일 문구] Single in-combat ally summary panel for a fantasy SRPG,
isolated on the panel only (no full screen), landscape. One wooden-framed slate
panel: top-left a chibi knight portrait in a square ornate slot frame; to its
right the unit name centered ("Knight"); below the name a wide HP bar with gold
frame and green fill showing "100/100"; under it two separate parchment stat
cells side by side — left cell "AP 10/10", right cell a speed icon with the
number 5; bottom-left a row of three square status-effect sockets, first one
holding a hand-painted buff icon, others empty. Green ally accent trim.
Clean spacing, no extra decorations, no banner text at the bottom.
```

## 4. 적 요약판 — **생성 불필요.** 시안(sian_04) 확정, 기존 에셋으로 조립함.

## (참고용 원본) 적 요약판 (전투 중 적 요약) — 1200x860 (판 하나만)

참고: 3번과 같은 배치 + `T_SkillIcon_BeastClaw.png`(다음 스킬 아이콘 톤)

```
[공통 스타일 문구] Single in-combat enemy summary panel for a fantasy SRPG,
isolated on the panel only, landscape. Same layout language as the ally summary
panel: top-left a chibi giant-eagle monster portrait in a square ornate slot
frame; unit name centered to its right ("독수리"); wide HP bar with gold frame
and RED fill showing "50/50"; two parchment stat cells "AP 0/5" and speed icon
with number 5; bottom-left a row of three square status sockets (empty); and at
the bottom-right ONE extra highlighted square socket showing the enemy's next
skill as a vivid hand-painted claw-strike icon with a subtle red glow — no
damage-preview numbers anywhere. Red enemy accent trim.
```

## 5. 스킬 상세 — **판 자체는 생성 불필요**(sian_05 확정, 기존 에셋 조립).
다만 작은 아이콘 2개만 따로 뽑아줘 (아래 5-1).

## (참고용 원본) 스킬 상세 (툴팁/팝업) — 900x1100 (세로 팝업)

참고: 위 공통 + `T_SkillIcon_*.png` 여러 장(아이콘 톤), `T_MB_HireSkillButtonFrame.png`

```
[공통 스타일 문구] Skill detail popup panel for a fantasy SRPG, isolated
vertical panel. Wooden-framed slate popup: at the top a large square skill
socket holding a vivid hand-painted whirlwind-slash skill icon; skill name
plate beside it ("회전 베기"); a parchment strip row of small stat cells —
AP cost with a gem icon ("AP 3"), range ("사거리 1"), target type icon; below
a large description area on dark slate with faint placeholder text lines and
one highlighted keyword chip; at the bottom a thin gold divider and a small
"닫기" button. Compact, readable, tooltip-like.
```

### 5-1. 소품 아이콘 (스킬/아티팩트 상세용 — 이것만 새로 생성) — 각 256x256, 투명 배경

```
[공통 스타일 문구] Small crisp UI glyph icon on transparent background,
hand-painted game-icon style, gold and dark-brown tones, reads at 48px:
(a) a crosshair / target-reticle glyph for "range"  — file: T_MB_Icon_Range
(b) a three-figures group glyph for "target: all"   — file: T_MB_Icon_TargetAll
(c) a small diamond-cut rarity gem, one gold version and one dark unlit version
    — files: T_MB_Gem_RarityOn / T_MB_Gem_RarityOff
```

## 6. 아티팩트 상세 — **생성 불필요.** 시안(sian_06) 확정, 기존 에셋 조립
(희귀도 젬만 5-1 의 (c) 사용).

## (참고용 원본) 아티팩트 상세 (툴팁/팝업) — 900x1100 (세로 팝업)

참고: 위 공통 + `T_Artifact_*.png` 6장(아이콘 톤), `T_MB_ArtifactSlot_Frame.png`

```
[공통 스타일 문구] Artifact detail popup panel for a fantasy SRPG, isolated
vertical panel. Wooden-framed slate popup: at the top a large ornate square
slot holding a hand-painted golden chalice artifact icon with faceted red gems;
artifact name plate beside it ("피의 성배") with a small rarity gem row
(3 of 5 gems lit gold); below, a parchment strip labelled "효과" and a
description area on dark slate with faint placeholder lines, one green
highlighted stat line ("+10%"); at the bottom a thin gold divider and a small
"닫기" button. Same frame language as the skill popup so they read as a family.
```

## 7. 설정판 — 1600x1000 (가운데 뜨는 큰 팝업, 모바일 게임 문법)

**모바일 게임 설정창 기준으로.** 줄마다 손가락으로 누를 만큼 큼직하게.
**아래 항목 목록은 게임에 실제로 배선돼 있는 전부다 — 하나도 빼먹지 말고,
없는 항목을 지어내지도 말 것.**

| 묶음 | 항목 (전부 표시) |
|---|---|
| 소리 | 전체 음량 슬라이더 · 배경음 슬라이더 · 효과음 슬라이더 · UI 사운드 슬라이더 |
| 화면 | 그래픽 품질 3단 세그먼트(낮음/중간/높음) · FPS 제한 2단(30/60) · 화면 흔들림 토글 · 전투 이펙트 토글 |
| 게임 | 진동 토글 · **언어 2단 세그먼트(한국어/English)** |
| 아래 단추 | 돌아가기(닫기) · 기본값 · [인게임 전용] 저장 후 종료 · 런 포기(붉은 테두리) |

참고: 위 공통 + `T_MB_OptionsRail_Frame.png`(왼쪽 레일 — 이미 게임에 있는 부품),
`T_KitA_Slider_Track.png`/`T_KitA_Slider_Fill.png`/`T_KitA_Slider_Knob.png`(슬라이더),
`T_KitA_Checkbox_On.png`/`_Off.png`(토글), `T_KitA_Button_Wide_Normal.png`(단추)

```
[공통 스타일 문구] Settings popup for a fantasy SRPG in mobile-game style,
isolated large centered panel, landscape 1600x1000. Wooden-framed slate panel
with a title plate ("설정") and a brass round close button top-right. LEFT: a
vertical category rail of 3 chunky square tab buttons with icons and labels —
소리, 화면, 게임 — first tab active with a gold glow. RIGHT: a scrollable list
of big touch-friendly parchment rows under small gold section headers, showing
ALL of these controls (top to bottom): [소리] 전체 음량 slider, 배경음 slider,
효과음 slider, UI 사운드 slider — each row = label + wooden slider track with
gold fill and brass knob + numeric value; [화면] 그래픽 품질 3-segment control
(낮음 / 중간 / 높음, middle segment lit gold), FPS 제한 2-segment (30 / 60),
화면 흔들림 chunky on/off toggle, 전투 이펙트 toggle; [게임] 진동 toggle,
언어 2-segment control (한국어 / English, 한국어 lit gold). A thin brass
scrollbar on the right edge. Bottom bar, left to right: wood "기본값" button,
gold "돌아가기" button, gold "저장 후 종료" button, and a red-trimmed
"런 포기" button. Chunky, generous spacing, every control obviously tappable,
nothing omitted from the list above.
```

## 8. 파츠 이미지 — 시안 확정 화면의 부품을 낱장으로

시안은 그림 한 장이라 그대로는 못 쓴다. **실제 WBP 텍스처로 쓸 부품 낱장**을 뽑아줘.

**전 파츠 공통 규칙 (매우 중요):**
- **글자 절대 넣지 말 것** — 글자는 엔진이 얹는다. 판·띠·단추 전부 빈 판으로.
- **아이콘도 넣지 말 것** — 아이콘은 별도 에셋으로 얹는다. 소켓·슬롯은 빈 구멍으로.
- 투명 배경 PNG(알파), 부품 하나당 한 장, 정면, 기울임·원근 없음.
- 늘려 쓰는 판/띠/단추는 **9-slice 가능하게**: 네 모서리 장식은 고정 폭,
  가운데와 변은 무늬 없이 균일하게 늘어나는 면으로.
- 빛 방향·질감 밀도는 기존 `T_MB_GenericDetailPanel.png` 과 같게 (한 게임으로 보여야 함).
- 파일명은 `T_MB_` 접두어 + 역할 (예: `T_MB_RosterCard_Frame.png`).

**뽑을 파츠 목록:**

용병탭(sian_01)에서:
```
T_MB_RosterCard_Frame      왼쪽 목록의 용병 카드 틀 (빈 판, 9-slice)
T_MB_RosterCard_Selected   위와 같되 초록 광 선택 상태
T_MB_LevelBadge_Plate      레벨 배지 작은 판 (글자 없이)
T_MB_ExpBar_Frame          EXP 바 틀 (가는 금테)
T_MB_ExpBar_Fill           EXP 바 채움 (금색, 늘려 쓰는 가로띠)
T_MB_EquipSlot_Frame       장비/아티팩트 정사각 슬롯 (빈 구멍)
```

몬스터탭(sian_02)에서:
```
T_MB_CodexCell_Frame       도감 격자 한 칸 틀 (빈 구멍, 9-slice)
T_MB_CodexCell_Locked      위와 같되 잠김(어두운) 상태 — 자물쇠·물음표 그리지 말 것
T_MB_TraitSocket_Frame     특성/스킬 소켓 (빈 구멍)
```

설정판(sian_07)에서:
```
T_MB_SettingsTab_Normal    왼쪽 레일 세로 탭 단추 (아이콘 없이 빈 판)
T_MB_SettingsTab_Active    위와 같되 금색 광 활성 상태
T_MB_SettingsRow_Plate     설정 한 줄 양피지판 (글자 없이, 9-slice)
T_MB_SectionHeader_Plate   묶음 제목 가는 금띠 (글자 없이)
T_MB_Toggle_On             켬 토글 (손잡이 오른쪽, 금색)
T_MB_Toggle_Off            끔 토글 (손잡이 왼쪽, 어두움)
T_MB_Segment_Frame         3칸 세그먼트 틀 (빈 칸 3개)
T_MB_Segment_Cell_Active   세그먼트 활성 한 칸 (금색)
T_MB_Dropdown_Box          내림 목록 상자 (화살표 홈만, 글자 없이)
T_MB_Scrollbar_Track       세로 스크롤 궤도 (가는 황동)
T_MB_Scrollbar_Thumb       스크롤 손잡이
```

적 요약판(sian_04) 보충 — 지금 공용 소켓으로 때우는 것들:
```
T_MB_StatusSlot_Enemy      상태 소켓의 붉은 장식판 (sian_04 의 각진 빨간 틀, 빈 구멍)
T_MB_NextSkillSocket       다음 스킬 소켓 (붉은 광 사각, 빈 구멍)
```

각 파츠의 생성 프롬프트 뼈대:
```
[공통 스타일 문구] Single isolated UI part on a fully transparent background,
no text, no icons, no letters, front-facing, no perspective. {부품 설명}.
Same wood-brass-slate material language and lighting as the reference panel.
Uniform stretchable middle area, ornate details only at corners/ends.
```

---

## 코덱스에게 마무리 부탁

- 전체 시안(1·2·7번)은 **한 장씩 어떤 화면인지 파일명에 적어서** 저장해줘 (예: `sian_01_용병탭.png`).
- 파츠(5-1·8번)는 위 파일명 그대로, 투명 배경 확인하고 저장.
- 톤 판정 기준은 딱 하나: `T_MB_GenericDetailPanel.png` + `T_MB_HireIcon_Knight.png` 옆에
  놓았을 때 한 게임으로 보이는가. 어긋나면 그 두 장을 참조 이미지(img2img/reference)로 걸고 다시.
- 판 계열은 같은 프레임 문법을 유지해야 한다 — 서로 딴 게임처럼 보이면 안 됨.
