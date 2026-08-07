# UI 컨셉 시안 요청서 (GPT-5.6-Sol)

작성 2026-08-04. 근거는 같은 날 실기 스크린샷 7장.

> **진행 상황 (2026-08-04 갱신)**
> 스타일 시트 3종과 부품 모음 3종을 받았고, **컨셉 A(나무·양피지)로 확정**했다.
> A 부품 18종은 잘라서 Unreal 에 넣었다 --
> `/Game/SVN/OutSideAsset/AICreation/UI/Marchbound/KitA/`.
> 남은 것은 **6절의 화면 시안(A-③~⑥)** 이다. 그건 아직 안 받았다.

---

## 1. 지금 무엇이 어긋났나

한 게임 안에 **서로 다른 화면 계열 네 개**가 섞여 있다. 배치를 아무리 고쳐도
계열이 넷이면 계속 중구난방으로 보인다. 배치가 아니라 **계열을 하나로 정하는
일**이 먼저다.

| 계열 | 쓰는 화면 | 바탕 | 테두리 | 글자색 |
|---|---|---|---|---|
| ① 어두운 석판 | 적 요약판, 용병 요약판 | 짙은 회청색 판 | 나무 + 금 리벳 | 흰색 |
| ② 나무 + 양피지 | 용병탭, 몬스터탭, 스킬·적·아티팩트 상세 | 크림색 양피지 | 갈색 나무 | 진갈색 |
| ③ 두루마리 + 책상 | 지도 | 양피지 두루마리 | 없음(책상 사진 위) | — |
| ④ 남색 + 금세공 | 설정 | 짙은 남색 | 가는 금선 | 흰색/금색 |

같은 뜻인데 모양이 다른 것들:

- **제목** — 몬스터탭·아티팩트 상세는 주황 판에 하늘색 발광 테두리, 용병탭은
  민무늬 가운데 글자, 설정은 발광 없는 큰 흰 글자. 셋이 한 게임처럼 안 보인다.
- **뒤로 단추** — 지도는 남색·금 알약, 용병탭·몬스터탭은 나무 판, 설정은 가는
  금색 사각. 같은 기능인데 세 가지다.
- **HP 표시** — 요약판은 굵은 빨강/초록 막대에 숫자를 얹고, 몬스터탭은 가는
  빨강 막대, 용병탭은 글자만. 규칙이 없다.
- **말** — 설정만 영어(Audio / Display / Master / Screen Shake)인데 그 안에
  "프레임"만 한국어로 섞여 있다. 나머지 화면은 전부 한국어다.

배치에서 드러난 빈 곳(내용이 아직 없어 비는 자리)도 시안이 답을 줘야 한다:

- 아티팩트 상세: 수치 칩 다섯이 전부 `-`, 오른쪽 열 통째로 "효과 설명이 아직
  없다" 한 줄. → **아티팩트는 수치 칩이 필요 없다.** 칩 자리를 아예 안 만드는
  배치가 필요하다.
- 몬스터탭: 슬라임은 스킬이 없어 오른쪽 열이 통째로 빈다. → **비었을 때
  무엇을 보여 줄지**를 시안이 정해 줘야 한다.

---

## 2. 공통 규칙 (세 컨셉 모두 지킬 것)

시안은 아래를 **고정**하고 재질·색만 바꾼다. 이걸 안 지키면 시안을 골라도
배치를 또 다시 짜야 한다.

- 화면 크기 **1920 × 1080**, 세로 화면 아님. 모바일이라 한 손 엄지가 닿는
  아래쪽 1/3 에 주요 단추를 둔다.
- **세 열 틀**을 쓰는 화면(스킬 상세 · 적 상세 · 아티팩트 상세 · 용병탭 ·
  몬스터탭)은 열 경계가 x38–499 / x518–1075 / x1094–1882, 위아래 y130–1015.
  그림에 칸을 그리려면 **이 선 위에** 그린다.
- 글자는 전부 **한국어**. 영어 이름(Knight, Slime)은 데이터라 그대로 둔다.
- 최소 글자 크기 24px(1080 기준). 폰이라 그 아래는 안 읽힌다.
- 단추는 최소 높이 66px, 서로 12px 이상 떨어뜨린다.
- 한 화면에 강조색은 **하나**만. 지금은 하늘색 발광과 주황 판이 같이 있어
  어디를 보라는 건지 모른다.

---

## 3. 컨셉 세 갈래

넷을 하나로 줄이되, **이미 만들어 둔 그림이 가장 많이 남는 쪽**부터 후보로
둔다. 괄호는 그 계열로 통일했을 때 버려야 하는 기존 그림의 대략적 양이다.

- **컨셉 A — 나무와 양피지** (버릴 그림 적음). 지금 탭·상세가 쓰는 계열.
- **컨셉 B — 어두운 석판과 황동** (중간). 지금 요약판이 쓰는 계열.
- **컨셉 C — 두루마리와 책상** (많음). 지금 지도가 쓰는 계열.

---

## 4. 프롬프트

각 컨셉마다 **여섯 장**을 받는다. ①스타일 시트 → ②부품 모음 → ③~⑥ 화면
시안. ①②를 먼저 받아 확정한 뒤 ③~⑥ 을 요청해야 결이 흔들리지 않는다.

프롬프트는 영어로 쓴다(그림 모델이 영어에 더 잘 붙는다). 한국어 글자는
"placeholder Korean text" 로 지시하고, 실제 문구는 우리가 나중에 넣는다.

---

### 컨셉 A — 나무와 양피지

**A-① 스타일 시트**

```
A game UI style sheet for a mobile fantasy tactics RPG, hand-painted 2D,
warm medieval workshop mood. Show, arranged as a labeled reference chart on a
neutral gray background:
- a color ramp of 6 swatches: aged parchment cream, mid oak brown, dark walnut,
  antique gold, deep ink brown, single accent of burnt orange
- three border treatments at different thicknesses: carved oak plank with
  visible grain, thin brass inlay line, rounded parchment edge with soft shadow
- two panel fills: flat cream parchment with subtle fiber texture, and a
  darker recessed well for contrast
- typography samples in a heavy rounded serif, ink brown on parchment
No text in English. Placeholder Korean glyphs only. Flat lighting, no gradients
stronger than 15%, no glow, no neon. Painterly but clean, readable at phone size.
```

**A-② 부품 모음**

```
A UI component kit sheet for the same wood-and-parchment fantasy game, laid out
in a labeled grid on neutral gray:
- primary button (oak plank with brass corner rivets) in normal / pressed /
  disabled states, wide pill proportion about 4:1
- secondary button, same family, smaller and flatter
- checkbox on and off, carved wood square with a brass tick
- horizontal slider: parchment track, brass knob, filled portion in burnt orange
- segmented control with 3 cells for quality levels
- a stat chip: small circular brass-rimmed disc for a number
- a horizontal bar for HP: recessed wood channel, dark red fill, and a second
  variant with green fill
- a title plate: carved wood banner, no glow
- a portrait frame: square, brass-cornered
All parts share the same border weight and corner radius. No glow, no neon rim.
```

**A-③ 세 열 화면 (상세·탭 공용 틀)**

```
A mobile game UI mockup, 1920x1080 landscape, wood-and-parchment fantasy style.
A full-screen carved oak frame divides the screen into three vertical parchment
panels of unequal width: narrow left (about 24%), medium center (29%), wide
right (41%), with a carved title banner centered along the top edge overlapping
the frame, not the panels.
Left panel: a large square portrait frame at top, a one-line caption under it,
then a block of body text filling to the bottom.
Center panel: a section heading, then five circular brass stat discs arranged
two-two-one, each with a small label above a large number.
Right panel: a section heading, then two 5x5 grids of small square cells side by
side, captions under each, then a two-row table of label-and-value pairs.
Every panel is filled to its bottom edge — no large empty regions.
Placeholder Korean text. No glow, no neon. Flat painterly lighting.
```

**A-④ 요약판 (적 · 용병 공용)**

```
A small floating info card for a mobile fantasy tactics game, about 640x470,
wood-and-parchment style. Carved oak frame with brass corner rivets around a
recessed dark panel. Top row: a square portrait frame on the left, a small
faction tag chip, and a name in large type. Below: a wide horizontal HP bar with
the value written on it. Below that, two side-by-side chips for action points
and speed, each with a small icon. Below that, a status row with two small
round status icons. A single footer strip across the bottom for one line of
text. Placeholder Korean text. No glow. Same border weight as the main frame.
```

**A-⑤ 지도**

```
A vertically scrolling world map screen for a mobile fantasy tactics game,
1920x1080 landscape, wood-and-parchment style. A tall aged parchment strip runs
top to bottom through a carved oak window frame; the parchment is plain and
tileable — faint terrain washes only, no painted houses or trees that would
smear when stretched. Oval node medallions connected by thin ink paths sit on
the parchment. Along the bottom, a carved oak bar holds two wide plank buttons
at the far left and far right, matching the button style of the rest of the
game. No desk, no candles, no gems, no photographic props.
Placeholder Korean text. No glow.
```

**A-⑥ 설정**

```
A settings screen for a mobile fantasy tactics game, 1920x1080 landscape,
wood-and-parchment style. A single carved oak frame holds one parchment sheet.
Inside, a centered carved title banner at top, then two equal columns of
setting rows. Each row is a shallow recessed parchment plate with a label on
the left and its control on the right: four rows with sliders in the left
column, then a group heading and one row with two selectable cells; the right
column has three rows with wooden checkboxes then two rows with segmented cell
groups. Along the bottom, four plank buttons in one row, evenly spaced.
Every row plate is the same height and border weight. Placeholder Korean text.
No blue, no gold filigree, no glow.
```

---

### 컨셉 B — 어두운 석판과 황동

A 의 여섯 장과 **같은 구도**로 받는다. 다른 것은 재질과 색뿐이다.
A-① ~ A-⑥ 의 프롬프트에서 스타일 문장만 아래로 바꾼다.

```
Replace the style clause with:
dark slate-and-brass style — panels are deep blue-gray slate with a matte
surface, frames are dark oiled wood with heavy brass rivets and corner plates,
text is warm off-white with a thin dark outline, the single accent is a muted
cyan. Recessed areas are near-black. No parchment anywhere.
```

이유: 지금 요약판이 이 계열이고, 전투 화면 위에 겹쳐 뜨는 판은 어두워야
아래 판이 비쳐도 글자가 읽힌다. 밝은 양피지 판을 전투 위에 띄우면 눈이
아프다 — 지금 아티팩트 상세가 그렇다.

---

### 컨셉 C — 두루마리와 책상

```
Replace the style clause with:
scribe's-desk style — every screen is a sheet of paper, vellum, or a wax-sealed
letter lying on a dark wooden desk; the desk surface with its ink pot, candle,
and scattered gems is visible around the edges of every panel. Panels have no
drawn frame — their own torn or rolled paper edge is the frame. Text is ink
brown. The single accent is sealing-wax red.
```

주의: 이 계열은 **화면마다 책상 소품이 달라지면 다시 중구난방**이 된다.
소품 배치를 한 장으로 고정해 달라고 따로 요청할 것. 그리고 판 테두리가
그림마다 달라지므로, 우리 쪽 좌표 계산이 가장 어려워지는 안이다.

---

## 5. 받은 뒤 할 일

1. 세 컨셉의 **①스타일 시트만** 먼저 놓고 하나를 고른다.
2. 고른 컨셉의 ②부품 모음을 확정한다. 여기서 단추 높이·테두리 두께·모서리
   반경이 정해지고, 그 숫자가 곧 배치 좌표가 된다.
3. ③~⑥ 을 받아 `Tools/UI/frame_registry.py` 의 칸 값을 다시 잰다
   (`catalog_frame_regions.py` 가 그림에서 분할선을 찾아 준다).
4. 그 뒤에 배치를 옮긴다. **순서를 바꾸면 배치를 두 번 짜게 된다.**


---

## 6. 다음 요청 — A안 화면 시안 (아직 안 받음)

부품 18종은 들어왔다. 하지만 **바탕을 이루는 큰 그림이 하나도 없다.** 지금
그대로 배치를 옮기면 새 단추가 옛 배경 위에 얹힌 꼴이 된다. 아래 다섯 장이
있어야 화면이 실제로 A안이 된다.

| 필요한 것 | 지금 그 자리에 있는 옛 그림 | 크기 |
|---|---|---|
| 세 열 대형 프레임 | `T_MT_BaseFrame` | 1920×1080 |
| 양피지 큰 판 (열 하나 채움) | 없음(단색 Border 로 때움) | 560×900 |
| 어두운 우물 (판 안 파인 자리) | 없음(단색 Border) | 512×512, 9-slice |
| 전체 화면 스크림 | `T_wm_panel_scrim` | 1920×1080 |
| 세로 스크롤 지도 몸통 | `T_StageMap_Background_Parchment` | 1024×3072 |

### 부품에서 잰 값 — 시안이 이 값을 지켜야 한다

부품 모음을 실측한 결과다(`Saved/UIKit/ConceptA/_nineslice.txt`). 화면 시안의
테두리 두께와 모서리 크기가 이 값에서 크게 벗어나면, 부품과 배경이 서로 다른
굵기로 보인다.

- 큰 단추 **764×164**, 테두리 두께 **44px**, 모서리 장식 **45px**
- 작은 단추 **281×133**, 테두리 **35px**
- 한 줄 받침 **526×140**, 테두리 **24px**
- 초상화 틀 **268×252**, 테두리 **34px**
- 제목 명패 **719×183**, 테두리 **50px**
- 체크박스 **136×133** · 슬라이더 손잡이 **102×104** · 수치 칩 테두리 **153×152**
  — 이 셋은 늘리지 않고 늘 같은 크기로 그린다

즉 **테두리 두께는 24~50px 사이**, 화면을 채우는 큰 그림도 이 범위여야 한다.

### A-③ 세 열 대형 프레임

**제목 명패는 넣지 말 것.** KitA 에 이미 따로 있다(`T_KitA_Title_Plate`, 719x183).
프레임에도 그리면 명패가 두 장 겹친다. 지금 화면의 주황 명패 + 하늘색 발광이
바로 프레임 그림에 박혀 있어서 코드로 못 떼는 것이다.

**분할선 좌표는 정확히 안 맞아도 된다.** 비율만 지키면 된다. 받은 그림을
`catalog_frame_regions.py` 로 다시 재서 칸 값을 갱신하고, 빌더는 그 값을 읽어
쓰기 때문이다. 억지로 픽셀을 맞추려다 그림이 어색해지는 쪽이 더 손해다.

```
A full-screen UI frame for a mobile fantasy tactics game, 1920x1080 landscape,
hand-painted wood-and-parchment style. It must match a component kit whose
borders are 24 to 50 pixels thick with brass corner brackets about 45 pixels
across.

A carved oak outer frame runs around the whole screen. Two vertical carved oak
dividers split the interior into three columns, left narrow, centre medium,
right wide, in roughly 24 / 29 / 41 proportion of the width. Each column is a
plain recessed opening — leave it empty, draw no content, no parchment sheet
inside, no headings, no icons.

Do NOT draw a title banner or name plate anywhere. The top edge is plain carved
frame.

Flat painterly lighting, warm oak and antique brass only. No glow, no neon rim,
no coloured light, no text. Transparent background outside the frame silhouette.
```

### A-④ 양피지 큰 판 · 어두운 우물

```
Two seamless UI fill textures for a wood-and-parchment fantasy game, on a
transparent background, as two separate labeled tiles:
1. an aged cream parchment sheet with soft fiber grain and slightly darkened
   edges, 560x900, meant to fill a tall column
2. a recessed dark well, near-black brown with a soft inner shadow along the top
   and left, 512x512, with a border thin enough to survive 9-slice stretching
   (about 40 pixels)
No frame, no ornament, no text. Flat lighting.
```

### A-⑤ 전체 화면 스크림

```
A full-screen overlay backdrop for a mobile fantasy game, 1920x1080, used behind
a popup panel. A dark warm-brown vignette, strongest at the corners, fading to
about 45 percent opacity at the centre so the battle below stays faintly
visible. Subtle carved wood texture at the very edges only.
No frame, no text, no icons. PNG with alpha.
```

### A-⑥ 세로 스크롤 지도 몸통

```
A tall vertically scrolling map background for a mobile fantasy game,
1024x3072 portrait. Aged parchment with faint ink-wash terrain — soft hills,
rivers, and forest hatching drawn lightly enough that stretching the image
vertically will not reveal distortion. No buildings, no landmarks, no roads with
hard edges, no text, no icons, no border. The edges must tile against themselves
top and bottom.
```

### 다섯 장 모두에 해당하는 것

- **발광 금지.** 지금 화면의 하늘색 발광은 그림에 박혀 있어 코드로 못 뗀다.
  같은 실수를 반복하면 또 못 뗀다.
- **글자·아이콘 금지.** 문구는 우리가 넣는다.
- **투명 배경(PNG).** 형태 밖은 알파 0 이어야 한다.
- 크기는 2의 거듭제곱에 가깝게. UI 텍스처라 밉을 안 만들지만 메모리는 아낀다.

주의: 지도 몸통은 **세로 3배 이상 늘어난 채로 쓰인다.** 지금 쓰는 941×1672
양피지는 비율이 0.56 이라 늘리면 티가 난다. 1024×3072(0.33)로 받아야 한다.
