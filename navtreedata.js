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
"EnemyUnit_8cpp.html",
"GameplayTagType_8cpp.html#a4026e1f177523d6a79372f94eeb41f42",
"GameplayTagType_8h.html#a29c91dd6047101f462476eb987d4c0d7",
"GameplayTagType_8h.html#affc8194ec54b9cb1d545815cc9363361",
"MockCombatDriver_8cpp_source.html",
"RDCollision_8h.html#aa246e00c1777bc381676776ca7c32afd",
"RewardConceptBoardBuilder_8cpp.html#aae33a801478725483f1968036341b16a",
"SRPGCommand_8h.html",
"SettingsPanelWidgetBuilder_8cpp.html#ac00b7f5319361e5167dcb749ac9b2e41",
"SkillConsoleCommand_8cpp_source.html",
"SkillTacticalDiagramWidgetBuilder_8cpp.html#a98fddca32654025ea8f4980a5d154f76",
"TacticalEffect_8cpp_source.html",
"TileMap_8h.html#a405a2b2ffa95581b9cb6792f17c83fea",
"WorldMapLandscapeWidgetBuilder_8h_source.html",
"classAShopGameMode.html#a5d1ad445c3eb6b372bd82f25a1a1dd16",
"classIOptionDataWriter.html",
"classUBoardActorAnimInstance.html#aacaf472a350b8b0f3a1e828e77bdbd33",
"classUCombatInputData.html#aee2bee4713f934ca0de573c798adabf5",
"classUEquipmentComponentModel.html#ab39266f57b3072dc989e3aeaad962b98",
"classUMercenaryHireWidget.html#a34bdfc1d61c8c5fac3f890c4ccac19f5",
"classUPlayerUnitModel.html#a088f3e31dcbbf07d8f0b502a46d94fbc",
"classURunPersistData.html#ad593b51fa2a8ce965070e6ef2e54d98a",
"classUSRPGTurnEndAction.html#a07a34d75c70c07199754bd6b875e4f89",
"classUShopUIWidgetBase.html#ad50aa6b022e0e450f3adb57e0432ef19",
"classUStaticCombatRoomSpawnData.html",
"classUTacticalEffect__ActionPoint.html",
"classUTacticalEffect__MaxHP.html#ab08e7c4df69e8db5535c7137c97aa3bd",
"classUUnitAttributeSet.html#a14ef9b036a52dfa2120becf97e94753c",
"functions_func_v.html",
"namespaceAnimationTags.html#a32e4882dc3f913200ddb7c4f3968cfee",
"namespaceAnimationTags.html#aba172cc4ca8bcb0205c3d7d15bd537e7",
"namespaceCombatHUDWidgetBuilder.html#af7b41bb03f090ae333fb7f830bf04a53",
"namespacePassiveConditionUtils.html",
"namespaceRoomPrimaryAssetTypes.html#a6c9c80ed3e538706e29d308552ee8856",
"namespaceTacticalEffectUtilities.html#a3f238d9a09bb1f78ba0507833dd133b9",
"structFActiveTacticalEffectsContainer.html#a9c55cb52c4ea217943dcc2c0f32b0b49",
"structFEquipmentDetailUI.html#a8fbf6f85a9729200508d769534415350",
"structFPassiveCondition.html#aa0da487c9570ae543d357d55dbc798f2",
"structFSRPGCombatRoundEventContainer.html#aa01d84ff731f73c6cb2430398ead54a5",
"structFSkillEffectLayer.html#aa52c4268c45be1fb3b2fc2ddcb251031",
"structFTacticalAttribute.html#a4ce2a6e6506c5eb3ce888e1f37769993",
"structFTreasureItemUI.html",
"structRewardConceptBoardBuilder_1_1FConceptSlot.html#af047fc5feca5d0632549ee9bfe8e0d44"
];

var SYNCONMSG = '패널 동기화를 비활성화하기 위해 클릭하십시오';
var SYNCOFFMSG = '패널 동기화를 활성화하기 위해 클릭하십시오';