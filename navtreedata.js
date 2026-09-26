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
    [ "타격 피드백 구현 계획", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html", [
      [ "</blockquote>", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md22", null ],
      [ "1. 실행 데미지 숫자 (최우선 — 코드 배선만으로 해결)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md23", null ],
      [ "2. 카메라 셰이크 (신규 에셋 = BP 2개)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md24", null ],
      [ "3. 히트 VFX / SFX (에셋 이미 있음 — 애님 배치 확인)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md25", null ],
      [ "4. WBP_CombatHUD04 위젯 이름 정정 (에디터 작업, 코드 무변경)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md26", null ],
      [ "5. 후순위 (이번 범위 밖, 기록만)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md27", null ],
      [ "진행 순서 제안", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2HIT__FEEDBACK__PLAN.html#autotoc_md28", null ]
    ] ],
    [ "전투 UI ↔ 게임플레이 경계 (View-Model 계약)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html", [
      [ "구성", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md30", [
        [ "다루는 도메인 (<tt>ECombatUIDomain</tt>)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md31", null ]
      ] ],
      [ "박용수(UI) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md32", null ],
      [ "모호재(게임플레이) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md33", null ],
      [ "범위 — 이건 '전투' 뷰모델", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md34", null ],
      [ "미합의/맞출 것", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2README.html#autotoc_md35", null ]
    ] ],
    [ "전투 UI ↔ 게임플레이 API 계약", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html", [
      [ "A. 게임플레이가 UI에 <strong>줘야 하는 것</strong> (gameplay → UI, <tt>Set*</tt>)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md38", [
        [ "예측/연출 큐 (게임플레이 → UI)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md39", null ],
        [ "빌드 종료 통지", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md40", null ]
      ] ],
      [ "B. UI가 게임플레이에 <strong>요구하는 것</strong> (UI → gameplay, <tt>Request*</tt> = 의도만)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md42", null ],
      [ "</blockquote>", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md43", null ],
      [ "C. UI가 <strong>구독하는 알림</strong> (게임플레이가 발신 → UI가 다시 그림)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md44", null ],
      [ "D. 게임플레이(모호재/김준형) 측 연결 지점 — 실제 배선", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md46", null ],
      [ "E. 아직 안 정해진 것 (게임플레이와 합의 필요)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md48", null ],
      [ "F. push vs pull — 방(비전투) 화면의 두 채널", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Combat_2UI__API__CONTRACT.html#autotoc_md50", null ]
    ] ],
    [ "전투 보상 화면 UI ↔ 게임플레이 경계 (View-Model 계약)", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html", [
      [ "구성", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md52", null ],
      [ "박용수(UI) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md53", null ],
      [ "모호재(게임플레이) 사용법", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md54", null ],
      [ "확장 메모", "md__2home_2runner_2work_2P__RD_2P__RD_2Source_2P__RD_2UI_2Reward_2README.html#autotoc_md55", null ]
    ] ],
    [ "네임스페이스", "namespaces.html", [
      [ "네임스페이스 목록", "namespaces.html", "namespaces_dup" ],
      [ "네임스페이스 멤버", "namespacemembers.html", [
        [ "모두", "namespacemembers.html", "namespacemembers_dup" ],
        [ "함수", "namespacemembers_func.html", "namespacemembers_func" ],
        [ "변수", "namespacemembers_vars.html", "namespacemembers_vars" ],
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
        [ "열거형 멤버", "functions_eval.html", null ],
        [ "관련된 함수들", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "파일들", "files.html", [
      [ "파일 목록", "files.html", "files_dup" ],
      [ "파일 멤버", "globals.html", [
        [ "모두", "globals.html", "globals_dup" ],
        [ "함수", "globals_func.html", "globals_func" ],
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
"CameraFunctionLibrary_8h_source.html",
"CombatTargetAttributeSet_8h_source.html",
"EnemyUnitModel_8cpp.html",
"GameplayTagType_8cpp.html#a37ef497ccaa24ada98e84190faa721e1",
"GameplayTagType_8h.html#a15cac7ecb7fe21d97d454d84448dfd93",
"GameplayTagType_8h.html#ae56ec7ac5ffa95ca71e1ea4766e8cbd2",
"MercenaryHireWidget_8cpp.html",
"PrimaryAssetType_8h.html#a04c47bb4e1ae251aa3ac4030e40ef4e7",
"RewardConcept03Widget_8cpp.html#ab0ab61c378abca693baa9a44cc43bdd0",
"SRPGAction_8h.html#a0566788d393db78a8bf98e456d9ef9fd",
"SettingsPanelWidgetBuilder_8cpp.html#a0ba98ec076c09e690c430b072bf09bce",
"SimulationSubsystem_8h.html#a66d3d444167260d0421bb6a4c5d92a3d",
"SkillEffectLayer__Vigor_8h_source.html",
"TacticalEffectQuery_8cpp.html",
"TextOpticalAlignment_8h.html#a20b34665fc26172bb3c5d99b42aa1b7a",
"WidgetTexturePurge_8h_source.html",
"classARDWorldSettings.html",
"classFTacticalTileTable.html#afd9ccc1056c3f3ab99864cdd42707e6d",
"classUAttributeSetComponentModel.html#a6a1672f8b99efc3dd9e3c03dd9b998e8",
"classUBossEntranceWidget.html#a6919b56d0cab4995d47b19167f214cff",
"classUCombatUIModel.html#afc382ed58d95dfffbcec8bb9f1322663",
"classUGameProfileSubsystem.html#a6dbad45bc7ad914beb6897b6bf86f16b",
"classUPartyArtifactComponentModel.html#ac757e05a8016d5b127719f99edd69d77",
"classURewardUIModel.html#a59994f6d3340bfe34a7b92bf5201f7df",
"classUSRPGDetailInfoPopupCommandHandler.html",
"classUShopUIWidgetBase.html#a0ae0f0b997d3d4ca75dc5e9d6671bd90",
"classUSkillDetailOverlayPresenter.html",
"classUTASActorModelMock.html#a775ea2a541debcd8238ea5cf98f6fdd0",
"classUTacticalEffect__GetDebuff__Acumeny.html",
"classUTileMapModel.html#a69338069b9cf1a0440bdb0b9633810af",
"dir_6cd1626ba92a5a6057b71e6a2a6ef174.html",
"namespaceAnimationTags.html#a02b5c6c55342a440a44c30278421329d",
"namespaceAnimationTags.html#a845b8d60e2cb98c170da2b5b4d5e2aad",
"namespaceAnimationTags.html#afc2817550897b0267bf5a15494589e40",
"namespaceFrontendMapPreview.html#aef993a2b66ccd56f857115d0ae0ae42d",
"namespaceRewardConcept03NewWidgetBuilder.html#ae57a19b0fd539336d008e67a86645bb8",
"namespaceShopPreview.html#afbdb55a9a40a5b870dd222a276b8cc89",
"namespacemembers_vars_p.html",
"structFCameraZoomEventTriggerPayload.html",
"structFLevelUpSkillReward.html#aaa3d859a547ead8aba744f2e22df51a9",
"structFRoom.html#a47dcdd84e642ef3a1f2a6d3bc3f05260",
"structFShopOwnedArtifactUI.html#a3763dbd86d21d9e17188454281151774",
"structFSkillUI.html#a3cd0b44e5f84a5e21d03b73867b38e50",
"structFTacticalEffectSpec.html#ae794452ab972eeaad52a45c7e9a3b2b7",
"structRewardConceptBoardBuilder_1_1FBSTextures.html#a079337d660a3294843234d49f428798a"
];

var SYNCONMSG = '패널 동기화를 비활성화하기 위해 클릭하십시오';
var SYNCOFFMSG = '패널 동기화를 활성화하기 위해 클릭하십시오';