# 전투 보상 화면 UI ↔ 게임플레이 경계 (View-Model 계약)

전투 뷰모델(`UI/Combat`)과 같은 패턴. 보상 화면은 **`URewardUIModel` 하나**로만 게임플레이와 만난다.
**현재 범위: 돈(골드) · 경험치만.** 아이템/주사위 보상은 이후 같은 패턴으로 도메인 추가.

```
[게임플레이] 전투 종료 → 보상 계산
      │ SetReward(FRewardUI)              ▲ OnRewardClaimed (받기 의도)
      ▼                                       │
            ┌────────  URewardUIModel  ────────┐
            └─────────────────────────────────────┘
      │ GetReward() / OnUIChanged
      ▼
[UI] URewardUIWidgetBase 상속 WBP — 돈/경험치 카운트업·막대 연출 + 받기 버튼
```

## 구성
| 파일 | 역할 |
|------|------|
| `RewardUITypes.h` | `FRewardUI` — 돈(번 양·잔액) + 경험치(번 양·레벨 전/후·막대 전/후·최대) |
| `RewardUIModel.h/.cpp` | 경계. `SetReward`/`GetReward`+`OnUIChanged`(읽기) · `RequestClaim`→`OnRewardClaimed`(주기) |
| `RewardUIWidgetBase.h/.cpp` | WBP 베이스. `BindUIModel` 후 `OnRewardRefreshed`만 구현, 버튼은 `Claim()` |
| `MockRewardDriver.h/.cpp` | 게임플레이 없이 가짜 보상 push + 받기 로그 — UI 선개발/테스트 |

## 박용수(UI) 사용법
1. WBP를 `URewardUIWidgetBase` 상속으로 만든다.
2. `BindUIModel(VM)` 호출.
3. `OnRewardRefreshed()`에서 `VM->GetReward()`를 읽어 돈/경험치 카운트업·막대 채움(전→후) 연출.
4. '받기' 버튼 → `Claim()`.
5. 게임플레이 전이라도 `UMockRewardDriver::Start(VM)`로 표시·연출·받기 테스트.

## 모호재(게임플레이) 사용법
Mock 자리에 어댑터만(위젯 무수정): 전투 종료 시 보상 계산 → `VM->SetReward(...)`(`PersistentData`의 골드/경험치 전/후),
`VM->OnRewardClaimed` 구독해 다음 화면(맵)으로 전환.

## 확장 메모
- 아이템/주사위 보상: `FRewardUI`에 배열 도메인 추가 + 선택 의도(`RequestPick(index)`)만 늘리면 됨(지금은 미구현).
- 레벨업 다단계: 현재 `mLevelBefore/After` + 한 막대(`mExpBefore→After`/`mMaxExp`). 다중 레벨업 연출이 필요하면 단계 배열로 확장.
