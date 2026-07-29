# 전투 밖 UI 배선 — develop 이 이미 쓰는 방식

> 작성: 박용수(UI) · 근거: `RDGameModeBase` · `RoomGameModeBase` · `FrontendGameMode` ·
> `WorldWidgetSubsystem` · `RDUserWidget` 를 직접 읽은 것
> **UI 를 새로 지어내지 않기 위해** 이미 오가는 규칙부터 적는다.

---

## 0. 한 줄

**전투와 전투 밖이 서로 다른 규칙을 쓴다.** 셋째 규칙을 만들면 안 된다.

| | 전투 | 전투 밖 |
|---|---|---|
| 값을 주는 쪽 | `UCombatUIModel` 이 **밀어** 준다 | 게임모드가 **꺼내 가게** 둔다 |
| UI 가 읽는 법 | `Get*()` + `OnUIChanged(도메인)` 구독 | `Get*View(OUT ...)` 를 그때 부른다 |
| UI 가 시키는 법 | `Request*()` → 델리게이트 | 게임모드 함수를 **바로 부른다** |
| 갱신 계기 | 게임플레이 사건마다 다시 밀림 | 화면이 열릴 때 · 눌렸을 때 |

전투는 **매 턴 값이 바뀌므로** 미는 편이 낫고, 전투 밖은 **누를 때만 바뀌므로**
꺼내 가는 편이 낫다. 둘 다 이유가 있다.

---

## 1. 화면은 서브시스템이 들고 있다

```
DefaultGame.ini   mWorldWidgetClasses[n] = 어느 WBP 인가
게임모드          mWorldWidgets 에 EWorldWidgetType 를 넣어 둔다
RDGameModeBase::BeginPlay
    → WorldWidgetSubsystem->InitWidgets(mHUDClass, mWorldWidgets)
```

**두 곳이 다 있어야 만들어진다.** ini 에 클래스를 걸어도 게임모드 목록에
없으면 안 만든다 -- 용병 게시판이 그래서 한 번 멈췄다.

`EWorldWidgetType` 의 값은 **ini 배열의 자리 번호**다. 가운데를 지우면 뒤가
전부 밀린다. 그래서 안 쓰는 자리도 `ReservedLegacy…` 로 비워 둔다.

### 화면 종류

| 갈래 | 무엇 |
|---|---|
| 알림 | `MsgNotify` · `SaveNotify` · `LoadingNotify` · `FadeInOut` |
| 공용 팝업 | `WorldMap` · `InGameSettings` · `SkillPanel` |
| 타이틀 | `CharacterSelect` · `MercenaryHire` |

**HUD 는 방마다 다르고, 월드 위젯은 방이 바뀌어도 같은 것을 쓴다.** 지도는
전투에서도 상점에서도 같은 인스턴스가 열린다.

---

## 2. 위젯 여는 규칙 하나

모든 화면이 `URDUserWidget` 을 물려받는다.

```cpp
OpenUI(콜백)    열림 연출 → FinishOpenUI() → 콜백
CloseUI(콜백)   닫힘 연출 → FinishCloseUI() → 콜백
```

**바깥에서 `SetVisibility` 를 부르지 않는다.** 연출이 있든 없든 같은 규칙으로
열고 닫으려고 이렇게 묶어 두었다. 애니메이션은 WBP 가 구현하고, 없으면
`FinishOpenUI()` 가 바로 불린다.

> 전투 HUD 도 이걸 물려받는다. `UCombatLayoutHUDWidget::OpenUI` 가
> `ApplyOpenUI` 를 덮어쓴 것은 **화면 전체를 덮어 클릭을 먹는 문제** 때문이다.

---

## 3. 값을 꺼내 가는 쪽 — 뷰 구조체

게임모드가 `OUT` 으로 채워 준다. UI 는 그 구조체만 알고 게임플레이 객체는
모른다.

| 함수 | 뷰 | 누가 쓰나 |
|---|---|---|
| `GetCharacterOptions(OUT)` | `FFrontendCharacterOption` | 용병 게시판 · 캐릭터 선택 |
| `GetMapRoomViews(OUT)` | `FMapRoomView` | 지도 |
| `GetRunControlView(OUT)` | `FRunControlView` | 이어하기 · 포기 단추 |

**전투의 `Set*` 과 짝이 되는 자리다.** 이쪽은 밀지 않고 물어보게 둔다.

---

## 4. 시키는 쪽 — 두 갈래

### 4.1 게임모드 함수를 바로 부른다

```cpp
World->GetAuthGameMode<ARoomGameModeBase>()->SelectNextRoom(행, 열);
```

`BlueprintCallable` 로 열어 둔 것들이다. **UI 는 결과를 만들지 않고 참/거짓만
돌려받는다.**

| 어디 | 무엇 |
|---|---|
| 타이틀 | `RequestCharacterSelectFromTitle` · `StartNewRun` · `ContinueRunFromTitle` · `AbandonRunFromTitle` |
| 방 | `SelectNextRoom` · `EnterSelectedRoom` · `AbandonRunFromRoom` |
| 공용 | 설정값 · 소리 · 화질 · 언어 (`RDGameModeBase`) |

### 4.2 위젯이 델리게이트로 올려준다

값이 여럿이거나 "다 골랐다" 같은 매듭은 델리게이트다.

```cpp
MercenaryHireWidget::mOnPartyConfirmed(TArray<FPrimaryAssetId>)
    → FrontendGameMode::HandlePartyConfirmed
    → StartNewRun(고른 셋, 난이도)
```

**한 번만 건다.** 서브시스템이 화면을 하나만 만들어 계속 쓰므로, 열 때마다
걸면 출발 한 번에 런이 여러 번 만들어진다 -- `mWasHireDelegateBound` 가 그것을
막는다.

---

## 5. 용병 고르기 흐름 — 처음부터 끝까지

```
타이틀 START
  → RequestCharacterSelectFromTitle()
  → OpenTitleCharacterSelect()
        GetCharacterOptions(OUT 후보)        게임모드가 값을 만든다
        HireWidget->SetCharacterOptions(후보, GetPartySize())
        (한 번만) mOnPartyConfirmed 에 붙는다
        TitleMenuWidget->CloseUI()
        HireWidget->OpenUI()

게시판에서 셋을 고르고 출발
  → mOnPartyConfirmed.Broadcast(고른 식별자 셋)
  → HandlePartyConfirmed()
  → StartNewRun(식별자 셋, 기본 난이도)
        CreateRunData()  →  방으로 전환
```

**게시판은 값을 모른다.** 후보도 인원도 받아서 그리기만 하고, 돌려주는 것도
식별자뿐이다. 그래서 나중에 상점이나 도중 합류에 같은 화면을 다시 쓸 수 있다.

> 캐릭터 선택 화면(`CharacterSelect`)은 지웠다가 되살렸다. 한 명만 고르는
> 자리가 다시 생길 수 있어서 남겨 두었다.

---

## 6. 방과 방 사이

```
지도에서 방을 고름   SelectNextRoom(행, 열)      고를 수 있나 판정도 게임모드
들어가기            EnterSelectedRoom()         프리로드 → 페이드 → 레벨 전환
```

**전환 중에는 다시 못 부른다**(`mWasNextRoomPreloadRequested`). 두 번 눌러
두 방으로 들어가는 일을 막는다.

페이드와 로딩 알림은 `RDGameModeBase` 가 잡는다 -- 방마다 따로 하지 않는다.

---

## 7. 그래서 UI 가 지켜야 할 것

1. **화면을 새로 만들면 `EWorldWidgetType` 에 넣고 ini 에도 건다.** 둘 중
   하나만 하면 조용히 안 만들어진다
2. **enum 가운데를 지우지 않는다.** ini 자리 번호라 뒤가 밀린다
3. **`OpenUI`/`CloseUI` 로만 열고 닫는다**
4. **전투 밖 값은 뷰 구조체로 꺼내 간다.** 새 `Set*` 을 만들지 않는다
5. **여럿을 돌려줄 때만 델리게이트.** 단추 하나는 게임모드 함수 직접 호출
6. **델리게이트는 한 번만 건다.** 화면은 하나를 계속 쓴다

---

## 8. 아직 안 맞춘 것

| 무엇 | 지금 |
|---|---|
| 상점 | `FShopItemUI` 는 있는데 전투 쪽 계약 자리에 있다. 전투 밖 방식과 안 맞음 |
| 보상 | `FRewardUI` 도 같은 자리 |
| 인벤토리 | 모델은 있고 화면이 없다 |
| 상단 메뉴 넷 | 눌러도 아무 일 없다. **무엇을 열지 안 정함** |

**넷 다 "어느 규칙으로 가나" 부터 정해야 한다.** 상점과 보상은 전투가 끝난
자리에서 열리므로 전투 쪽 규칙이 편할 수 있고, 인벤토리는 방 어디서나 열리므로
월드 위젯 + 뷰 구조체가 맞다.
