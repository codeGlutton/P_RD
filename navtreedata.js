/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "P_RD", "index.html", [
    [ "P_RD 프로젝트", "index.html", null ],
    [ "Primary Asset Type 매크로", "priamry_asset_type_page.html", [
      [ "Primary Asset Type 매크로란?", "priamry_asset_type_page.html#primary_asset_type_macro_section", null ],
      [ "필요성", "priamry_asset_type_page.html#primary_asset_type_need_section", null ],
      [ "사용법", "priamry_asset_type_page.html#primary_asset_type_use_section", [
        [ "생성 시", "priamry_asset_type_page.html#create_primary_asset_type_step1", null ],
        [ "사용 시", "priamry_asset_type_page.html#use_primary_asset_type_step2", null ]
      ] ]
    ] ],
    [ "Gameplay 태그 매크로", "gas_tag_page.html", [
      [ "태그 매크로란?", "gas_tag_page.html#tag_macro_section", null ],
      [ "필요성", "gas_tag_page.html#tag_need_section", null ],
      [ "사용법", "gas_tag_page.html#tag_use_section", [
        [ "생성 시", "gas_tag_page.html#create_tag_step1", null ],
        [ "사용 시", "gas_tag_page.html#use_tag_step2", null ]
      ] ]
    ] ],
    [ "Model-View 프레임워크", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html", [
      [ "1. 이원화 컨텍스트 및 동작 모드 (USimulationSubsystem)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md2", [
        [ "🔄 컨텍스트 관리 구조", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md3", null ]
      ] ],
      [ "2. 생성 단계 (Creation Phase)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md5", [
        [ "🔄 생성 동작 비교", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md6", null ],
        [ "🔄 생성 시퀀스 다이어그램 (인게임 모드 기준)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md7", null ],
        [ "💡 생성 단계 상세 흐름", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md8", null ]
      ] ],
      [ "3. 소멸 단계 (Destruction Phase)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md10", [
        [ "🔄 소멸 시퀀스 다이어그램 (인게임 모드 기준)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md11", null ],
        [ "💡 소멸 단계 상세 흐름", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md12", null ]
      ] ],
      [ "4. 컴포넌트 모델 구조 (UActorModel & UComponentModel)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md14", null ],
      [ "5. 시뮬레이션 결과 기록 시스템 (UEventLogger)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md16", null ]
    ] ],
    [ "SRPG 프레임워크 흐름", "srpg_framework_page.html", [
      [ "SRPG 싱글톤 객체", "srpg_framework_page.html#srpg_framework_subsystem_image_section", [
        [ "Q 초기 턴 순서 배치는 어떻게 처리하는가?", "srpg_framework_page.html#srpg_framework_subsystem_step1", null ],
        [ "Q 턴 추가 및 삭제 확장성이 있는가?", "srpg_framework_page.html#srpg_framework_subsystem_step2", null ],
        [ "Q 액션 큐는 항상 수동 추가인가?", "srpg_framework_page.html#srpg_framework_subsystem_step3", null ],
        [ "Q 턴 종료는 어디서 검사하는가?", "srpg_framework_page.html#srpg_framework_subsystem_step4", null ]
      ] ],
      [ "단일 턴에 대한 순서도", "srpg_framework_page.html#srpg_framework_turn_image_section", [
        [ "공격 스킬 순서 상세 예시", "srpg_framework_page.html#srpg_framework_turn_step1", null ]
      ] ]
    ] ],
    [ "전투 UI ↔ 게임플레이 경계 (View-Model 계약)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html", [
      [ "구성", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md18", [
        [ "다루는 도메인 (<tt>ECombatUIDomain</tt>)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md19", null ]
      ] ],
      [ "박용수(UI) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md20", null ],
      [ "모호재(게임플레이) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md21", null ],
      [ "범위 — 이건 '전투' 뷰모델", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md22", null ],
      [ "미합의/맞출 것", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md23", null ]
    ] ],
    [ "전투 UI ↔ 게임플레이 API 계약", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html", [
      [ "A. 게임플레이가 UI에 <strong>줘야 하는 것</strong> (gameplay → UI, <tt>Set*</tt>)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md26", [
        [ "예측/연출 큐 (게임플레이 → UI)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md27", null ],
        [ "빌드 종료 통지", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md28", null ]
      ] ],
      [ "B. UI가 게임플레이에 <strong>요구하는 것</strong> (UI → gameplay, <tt>Request*</tt> = 의도만)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md30", null ],
      [ "</blockquote>", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md31", null ],
      [ "C. UI가 <strong>구독하는 알림</strong> (게임플레이가 발신 → UI가 다시 그림)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md32", null ],
      [ "D. 게임플레이(모호재/김준형) 측 연결 지점 — 무엇을 어디에 물릴지", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md34", null ],
      [ "E. 아직 안 정해진 것 (게임플레이와 합의 필요)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md36", null ]
    ] ],
    [ "전투 보상 화면 UI ↔ 게임플레이 경계 (View-Model 계약)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html", [
      [ "구성", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md42", null ],
      [ "박용수(UI) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md43", null ],
      [ "모호재(게임플레이) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md44", null ],
      [ "확장 메모", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md45", null ]
    ] ],
    [ "네임스페이스", "namespaces.html", [
      [ "네임스페이스 목록", "namespaces.html", "namespaces_dup" ],
      [ "네임스페이스 멤버", "namespacemembers.html", [
        [ "모두", "namespacemembers.html", null ],
        [ "함수", "namespacemembers_func.html", null ],
        [ "변수", "namespacemembers_vars.html", null ],
        [ "열거형 타입", "namespacemembers_enum.html", null ],
        [ "열거형 멤버", "namespacemembers_eval.html", null ]
      ] ]
    ] ],
    [ "클래스", "annotated.html", [
      [ "클래스 목록", "annotated.html", "annotated_dup" ],
      [ "클래스 색인", "classes.html", null ],
      [ "클래스 계통도", "hierarchy.html", "hierarchy" ],
      [ "클래스 멤버", "functions.html", [
        [ "모두", "functions.html", "functions_dup" ],
        [ "함수", "functions_func.html", "functions_func" ],
        [ "변수", "functions_vars.html", "functions_vars" ],
        [ "타입정의", "functions_type.html", null ],
        [ "관련된 함수들", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "파일들", "files.html", [
      [ "파일 목록", "files.html", "files_dup" ],
      [ "파일 멤버", "globals.html", [
        [ "모두", "globals.html", "globals_dup" ],
        [ "함수", "globals_func.html", null ],
        [ "변수", "globals_vars.html", null ],
        [ "열거형 타입", "globals_enum.html", null ],
        [ "매크로", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"ActiveTacticalEffect_8cpp.html",
"CombatTileMapHUDWidget__RuntimeWidgets_8cpp_source.html",
"FrontendGameMode_8cpp.html",
"PassiveComponentModel_8cpp.html",
"SRPGCombatModel_8h.html#a461953f75a5da19674edeb1fc2337eb5",
"SkillComponentModel_8h.html#a3ed22cbe65383bb490a979927fb5706d",
"TacticalEffect__AttackFactor__Override_8cpp_source.html",
"UnitAttributeSet_8h.html",
"classATileMap.html#a1acd6095836c59755f9b2f898a186a3e",
"classUAttributeSetComponentModel.html#af7298699406563c5c85ba790e4b43f56",
"classUCombatUIWidgetBase.html#aec7e3913aa2ab065e7f357dd57f82ffa",
"classULevelAttributeSet.html",
"classURunDataWriter.html",
"classUSaveGameSubsystem.html#a58d0bc72951e46a39cce69c9db9e25ea",
"classUStaticTreasureRoomSpawnData.html",
"classUTitleBackgroundVideoSubsystem.html#a8f70473289a4e2fec28950cffe2002f9",
"md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2ModelViewFramework.html#autotoc_md5",
"pages.html",
"structFGlobalStatusEffectSetting.html#aeb544becfcf2c218e505ead66ce708fc",
"structFShopItemUI.html#aeb24f9c5efee5d0c695a28553c6e20fc",
"structFTacticalEffectSpec.html#a8b26175466a4dd817dbb41f23529b3f5"
];

var SYNCONMSG = '패널 동기화를 비활성화하기 위해 클릭하십시오';
var SYNCOFFMSG = '패널 동기화를 활성화하기 위해 클릭하십시오';