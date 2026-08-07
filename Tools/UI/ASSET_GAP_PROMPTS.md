# 시안 50종에서 채우지 못한 에셋과 생성 프롬프트

`Tools/UI/build_screen_variants.py` 로 만든 10화면 × 5안은 **모두 기존 에셋만** 썼다.
아래는 적절한 기존 에셋이 없어 **빈 칸(점선)으로 남겨 둔 자리**와, 그 자리를 채울
이미지를 만들 때 쓸 프롬프트다. 프레임·판·버튼 같은 큰 그림은 이미 있으므로
필요한 것은 대부분 **아이콘**이다.

## 공통 스타일 지시문 (모든 프롬프트 앞에 붙일 것)

```
Mobile game UI icon for a low-poly chibi fantasy tactics roguelike.
Hand-painted stylized look, warm parchment-and-gold "Marchbound" theme,
thick readable silhouette, soft rim light, no text, no letters, no numbers.
Centered subject, transparent background (PNG with alpha), square canvas,
flat front-facing presentation suitable for a UI slot, not a scene.
```

- 출력: **PNG, 알파 채널 필수**, 정사각 **512×512** (뱃지/작은 아이콘은 256×256)
- 저장 위치: `Content/SVN/OutSideAsset/AICreation/UI/<폴더>/` (SVN 폴더 규칙 유지)
- 임포트 후 텍스처 압축은 기존 UI 텍스처 설정을 따라갈 것

---

## 1. 스킬 아이콘 (가장 시급)

**현황**: `CombatHUD/SkillIcons/` 에 `T_CombatHUD_SkillIcon_1~4`, `_Basic`, `_Step`
여섯 개뿐. 스킬은 이미 그보다 많고(베기·강타·연속 찌르기·회전베기·도약·포효·
야수의 발톱 등), 그래서 전투 화면에서 **회전베기에 부츠 아이콘이 붙는** 오배정이
생겼다. 스킬 1종 = 아이콘 1종이 되어야 한다.

**저장**: `UI/CombatHUD/SkillIcons/T_SkillIcon_<이름>`

| 스킬 | 프롬프트 (공통 지시문 + 아래) |
|---|---|
| 베기 | `a single straight sword slash, one clean diagonal blade arc with a steel sword crossing it` |
| 강타 | `a heavy overhead smash, a broad sword driven down with impact sparks and a cracked ground line` |
| 연속 찌르기 | `three parallel thrusting blades stabbing forward, motion streaks behind the points` |
| 회전베기 | `a circular spinning slash, a ring-shaped blade trail around a centered sword` |
| 도약 | `a leap arc, a curved dotted jump trajectory over a small landing burst` |
| 포효 | `a roaring shout, concentric sound rings expanding from an open fanged maw` |
| 야수의 발톱 | `three curved beast claws tearing forward with torn slash marks` |
| 방벽 | `a raised tower shield with a glowing protective barrier plane in front` |
| 돌진 | `a charging forward dash, a shield-first silhouette with speed streaks` |
| 평타 | `a simple basic attack mark, a short sword and a small impact star` |

---

## 2. 아티팩트 아이콘

**현황**: 카드 판(`Reward/T_Reward_ArtifactCard_V1/V2`, `_Rare_V2`, `_Sel_V*`)과
슬롯 틀(`Marchbound/Combat/T_MB_ArtifactSlot_Frame`)은 있으나 **아티팩트 자체의
그림이 하나도 없다**. 아티팩트 상세 5안의 아이콘 칸이 전부 비어 있다.

**저장**: `UI/Artifacts/T_Artifact_<이름>`

| 아티팩트 | 프롬프트 (공통 지시문 + 아래) |
|---|---|
| 이빨 부적 | `a fang amulet, several wolf teeth bound with leather cord into a talisman` |
| 낡은 방패 장식 | `a battered heraldic shield boss, dented bronze with a worn emblem` |
| 행운의 주사위 | `a single carved bone die showing a lucky face, faint golden glow` |
| 피의 성배 | `an ornate goblet half filled with dark red liquid, ruby set in the stem` |
| 여행자의 지도 | `a rolled parchment map fragment tied with twine, compass rose visible` |
| 가시 문장 | `a thorned iron sigil, sharp barbs radiating from a dark crest` |

> 6종은 최소 세트다. 아티팩트 수만큼 계속 늘려야 하므로 **한 번에 시트로 뽑지 말고
> 개별 파일**로 만들 것 — 슬롯마다 따로 물려야 한다.

---

## 3. 직업 심볼 (누락분)

**현황**: `ClassSelect/T_class_symbol_knight_v2 / _mage_v2 / _rogue_v2` 세 개만 있다.
용병은 여섯 직업(기사·마법사·레인저·도적·야만전사·드루이드)이라 **세 개가 빈다**.
용병탭·용병 요약의 직업 표시 자리가 그래서 비어 있다.

**저장**: `UI/ClassSelect/T_class_symbol_<직업>_v2` (기존 3종과 같은 규격)

| 직업 | 프롬프트 (공통 지시문 + 아래) |
|---|---|
| 레인저 | `a ranger class emblem, a longbow crossed with a feathered arrow inside a rounded crest` |
| 야만전사 | `a barbarian class emblem, a double-headed axe over a fur-trimmed round crest` |
| 드루이드 | `a druid class emblem, an antler-and-leaf motif inside a rounded wooden crest` |

---

## 4. 상태이상 아이콘 (누락분)

**현황**: `CombatHUD/StatusIcons/` 에 8종(민첩·강화·방어 획득·이동 획득·피해·회복·
취약·약화)이 있다. 시안에서 쓴 상태 중 아래가 없다.

**저장**: `UI/CombatHUD/StatusIcons/T_Status_<이름>`

| 상태 | 프롬프트 (공통 지시문 + 아래) |
|---|---|
| 출혈 | `a bleeding status mark, three falling blood droplets over a thin cut line` |
| 중독 | `a poison status mark, a skull-shaped bubble rising from a green droplet` |
| 기절 | `a stun status mark, small stars circling above an impact point` |
| 은신 | `a stealth status mark, a faded hooded silhouette dissolving into smoke` |

---

## 5. 등급 뱃지

**현황**: `Common/KK_Badge_Round` 하나뿐이라 일반/희귀/영웅을 색으로만 구분해야 한다.
스킬·아티팩트 상세에 등급 표시가 들어가므로 세 종류가 필요하다.

**저장**: `UI/Common/T_Badge_Grade_<등급>` · 256×256

```
<공통 지시문>
a small circular rarity badge frame, ornate metal rim, empty center for a letter,
<등급별>: common = plain iron grey rim
          rare  = polished blue-steel rim with two small gems
          epic  = gold rim with purple gems and subtle radiating glow
```

---

## 6. 타이틀 버튼 상태 (선택)

**현황**: `Title/T_menu_button_frame_normal` 만 있고 hover/pressed/disabled 가 없다.
모바일이라 hover 는 덜 중요하지만 **pressed 와 disabled 는 필요하다**
(CONTINUE 는 세이브가 없으면 비활성이어야 한다).

**저장**: `UI/Title/T_menu_button_frame_pressed` / `_disabled`

```
<공통 지시문>
a horizontal fantasy menu button plate, ornate gold trim on dark wood,
wide 4:1 aspect, empty center for a label,
<상태별>: pressed  = pushed in, darker face, inner shadow at top
          disabled = desaturated grey-brown, dull trim, no glow
```

---

## 만들지 않아도 되는 것 (기존 에셋으로 충분)

- **설정**: 슬라이더(track/fill/knob), 체크박스(on/off), 탭 틀·탭 아이콘 3종,
  세그먼트, 적용/닫기/일반/비활성 버튼까지 `UI/Settings/` 에 완비돼 있다.
- **지도**: `RunFlow/` 의 V2 노드 6종·경로 2종·범례·팝업 배경, `WorldMap/` 의
  링 4종·마커 2종·연결선, `MapNode/` 아이콘 20종으로 충분하다.
- **초상화**: `Portraits/` 에 41종(직업 + 적)이 Head/Action 두 벌로 있다.
- **패널·틀**: `Marchbound/` 68종에 전면 프레임(16:9)·행·칩·슬롯·버튼이 다 있다.
