# Combat playback speed

- Combat HUD: double-chevron icon + 1x / 2x / 3x, cycling back to 1x. Existing small-button art is hard referenced; chevrons are drawn without a font or texture dependency.
- HUD sends an intent through CombatUIModel. CombatPlaybackComponent on CombatGameMode owns one indefinite request in the camera TimeScaleComponent and stores the selected integer in OptionPersistData.
- The persistent request multiplies with independent skill time-scale requests. Replacing the selected value does not release cinematic handles or stack additional playback handles.
- Start after boss entrance; stop at combat result or EndPlay. Reattach if the main camera changes. Result/next-room UI does not retain the playback multiplier.
- Menus and tutorial reading stay on normal UI time. Skill cut-ins and floating combat feedback explicitly follow the selected playback speed; world camera movement/shake uses world delta.
- Existing option saves default to 1x. Save failures use the existing save-status/retry path.

Validation: P_RD.Combat.Playback (mixer composition, combat lifecycle, 1-2-3-1 HUD wiring, option serialization, rendered Fold and widescreen layouts). P_RD.Tutorial.Guided and P_RD.Tutorial.Shop regression tests also pass.
Local Android test package: com.aurelight.mercenaryguildoftheruinedkingdom.tutorialtest, versionCode 6. No Play upload.
