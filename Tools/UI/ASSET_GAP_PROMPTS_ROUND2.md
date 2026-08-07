# 2차 에셋 요청 — 상세/요약 화면의 프레임과 세부 파츠

배치가 통과된 화면(스킬 상세 · 적 요약 · 용병 요약)에서 **지금 남의 화면 것을
빌려 쓰고 있는 자리**만 모았다. 배치를 바꾸자는 게 아니라, 임시로 끼워 둔 그림을
전용 그림으로 갈아 끼우기 위한 요청이다.

## 지금 무엇을 빌려 쓰고 있나

| 자리 | 지금 쓰는 것 | 문제 |
|---|---|---|
| 상세창 전면 판 | `T_MT_BaseFrame` (몬스터탭 것) | 상세·용병탭·몬스터탭이 **같은 배경**이라 화면 구분이 색 틴트뿐 |
| 수치 칩 | `T_MB_StatusSlot_Frame` (상태 슬롯 것) | 상태 아이콘 홀더라 수치용으로는 테두리가 과함 |
| 범위 칸 | 단색 사각형 (그림 없음) | 판의 타일 느낌이 안 남 |
| 요약판 | `T_MB_GenericDetailPanel` | 적/아군 구분이 색 하나뿐 |

---

## 공통 스타일 지시문 (모든 프롬프트 앞에 붙일 것)

```
UI frame art for a low-poly chibi fantasy tactics roguelike, mobile game.
Hand-painted stylized look, warm parchment-and-gold "Marchbound" theme,
carved wood and aged brass, soft inner shadow, no text, no letters, no numbers,
no characters, no icons inside — the interior must stay EMPTY and flat so UI
text and widgets can be laid on top. Straight-on orthographic view, no perspective.
Transparent background (PNG with alpha).
```

> **가장 중요한 지시**: 안쪽은 **비워야** 한다. 지금 겪은 문제가 그림에 칸이
> 이미 그려져 있는데 그걸 모르고 위젯을 얹어 전부 어긋난 것이었다.
> 칸을 그리려면 **어디가 칸인지 아래 비율대로** 그려 달라고 명시해야 한다.

---

## 1. 상세창 전용 3열 판 (● 필수)

**저장**: `UI/Marchbound/Common/T_MB_DetailBase_3Col` · **1920×1080 (16:9)**

```
<공통 지시문>
a full-screen tactics UI backdrop split into THREE vertical panels by two carved
wooden dividers. Panel widths from left to right are 24%, 29%, 41% of the frame.
The three interiors are flat empty parchment. A horizontal wooden band runs across
the top 12% for a title, with a decorative name plate centered in it.
The bottom 6% is a plain wooden rail. Cool blue-grey wood with brass corners,
distinct from the warm brown monster-tab frame.
```

**왜 이 비율인가**: 현재 `T_MT_BaseFrame`을 실측한 값(좌 0.02~0.26 / 중 0.27~0.56 /
우 0.57~0.98)과 같게 맞춰야 코드 수정 없이 갈아 끼울 수 있다. 제목판도 지금과 같은
자리(가로 0.17~0.51, 세로 0.06~0.18)에 둬야 한다.

---

## 2. 수치 칩 배경 (● 필수)

**저장**: `UI/Marchbound/Common/T_MB_StatChip` · **256×256 정사각**

```
<공통 지시문>
a small round stat medallion, thin brass ring on dark slate fill,
subtle bevel, empty flat center with room for two short lines of text.
Simpler and lighter than a status-effect socket — this holds numbers, not icons.
```

---

## 3. 범위 칸 타일 4종 (● 필수)

**저장**: `UI/Marchbound/Common/T_MB_RangeCell_<종류>` · **128×128 정사각**

지금은 단색 네모라 판 위의 타일과 느낌이 다르다. 네 상태를 색만 다르게 뽑는다.

| 종류 | 프롬프트 (공통 지시문 + 아래) |
|---|---|
| `Empty` | `an empty grid tile, dark slate stone with a faint chiseled border` |
| `Aim` | `a highlighted grid tile glowing cool blue, thin bright rim, translucent fill` |
| `Hit` | `a highlighted grid tile glowing warm red, thin bright rim, translucent fill` |
| `Caster` | `a grid tile marked as the caster's own square, golden rim with a small centered diamond notch` |

---

## 4. 요약판 프레임 2종 (△ 권장)

**저장**: `UI/Marchbound/Common/T_MB_SummaryPanel_Ally` / `_Enemy` · **1536×1024 (3:2)**

전투 중 옆에 뜨는 작은 판이다. 지금은 아군·적이 같은 그림에 색만 달라 구분이 약하다.

```
<공통 지시문>
a compact side information panel, 3:2, flat empty interior,
a narrow title band across the top 15%,
<아군>: friendly tone — polished steel rim with blue enamel inlay
<적>:  hostile tone — dark iron rim with red enamel inlay and a subtle jagged edge
```

---

## 5. HP 바 3피스 (△ 권장)

**저장**: `UI/Marchbound/Common/T_MB_HpBar_Track` / `_Fill` / `_Frame`

```
<공통 지시문>
a horizontal health bar piece, 8:1 aspect,
<Track>: empty recessed groove, dark and hollow
<Fill>:  solid bright fill strip, flat colour so it can be tinted
<Frame>: thin brass casing that overlays the groove, transparent center
```

세 장으로 나눠야 채움 폭만 바꿔 줄일 수 있다. 한 장으로 뽑으면 못 쓴다.

---

## 만들지 않아도 되는 것

- **제목판·닫기 버튼**: `T_MB_HireTitlePlate` / `T_MB_HireBackButton` 로 충분하다.
- **초상화 틀**: `T_RS_PortraitFrame` 이 정사각이라 그대로 쓴다.
- **스킬 아이콘·상태 아이콘·아티팩트**: 1차 요청분 28종으로 지금은 충분하다.
