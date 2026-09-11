# Android builds and release validation

The project still supports ordinary development APK packaging. Distribution settings and signing credentials are applied only to a disposable project copy by `build_release.py`; the working project is not switched permanently to release mode.

## Compilation and CI

```powershell
$env:UE_ROOT = 'C:\Program Files\Epic Games\UE_5.7'
.\Tools\Android\compile.ps1
python Tools/Android/check_sources.py
python -m unittest discover -s Tools/Android/tests -v
```

`compile.ps1` builds the actual Android arm64 Shipping game target. It needs an installed UE 5.7 engine with Android support, the matching Android SDK/NDK/JDK, and normal Unreal build prerequisites. It does not cook assets or sign packages.

The `Android verification` workflow runs source checks and release-validator unit tests on GitHub-hosted Windows for every PR to develop/main, including Draft PRs. Native compilation requires an isolated Windows x64 self-hosted runner with UE 5.7/Android installed and `UE_ROOT` configured. Set repository variable `ANDROID_CI_RUNNER` to that runner's additional label. Do not put release signing credentials on the PR runner. Fork PRs only run hosted source checks until their code is reviewed and built in a trusted branch.

Native compilation is optional while no runner is configured: the job is skipped without failing the workflow. The separate `Android native gate` has been removed. Source checks and release-validator tests still run; a configured native build still reports its actual success or failure. A source-only pass does not mean Android was compiled; local APK builds provide that verification until CI is configured. This change does not edit branch rules or register a runner automatically.

At implementation time (2026-09-08), the repository runner inventory API returned HTTP 403, so an available native runner could not be verified. The pre-existing develop rule required two approving reviews and last-push approval, but no Android status check.

## Signed AAB release

Prerequisites:

- Python 3.11+, Windows, UE 5.7, its Android toolchain, Android SDK build-tools 35+.
- Full `Content/SVN` checkout at the intended release revision. Git alone does not include these assets.
- A local verified `bundletool` JAR; UE 5.7 bundles version 1.18.1. Pass `--bundletool` to use a different reviewed version.
- An existing upload keystore and its expected certificate SHA-256 fingerprint. Obtain these from the release owner/secret store. The tool never creates a production key.
- Enough temporary disk space for a full project input copy plus cooked output. SVN metadata and generated caches are not copied.

Provide these variables through the local/CI secret store, rather than writing passwords in a script, command history, INI committed to Git, or PR:

| Environment variable | Meaning |
| --- | --- |
| `UE_ROOT` | Installed engine root |
| `ANDROID_HOME` | Android SDK root (`ANDROID_SDK_ROOT` is also accepted) |
| `JAVA_HOME` | JDK root, including java/keytool/jarsigner |
| `RD_ANDROID_UPLOAD_KEYSTORE` | Path to the existing upload keystore |
| `RD_ANDROID_UPLOAD_ALIAS` | Upload private-key alias |
| `RD_ANDROID_UPLOAD_STORE_PASSWORD` | Keystore password |
| `RD_ANDROID_UPLOAD_KEY_PASSWORD` | Private-key password; defaults to the store password |
| `RD_ANDROID_UPLOAD_CERT_SHA256` | Expected public upload certificate fingerprint, 64 hex digits, optional colons |

Unreal writes signing credentials into single-quoted Gradle strings. This script fails before building if the alias/password contains quotes, backslashes or line breaks that this UE generation path cannot encode safely. Resolve that signing/toolchain constraint before attempting release; there is no debug-key fallback.

```powershell
# First checks tools, external SVN assets, private-key type and pinned certificate.
python Tools/Android/build_release.py --version-code 2 --preflight-only

# Use a NEW output directory and a version code not previously uploaded to Play.
python Tools/Android/build_release.py --version-code 2 --output outputs/android-release-v2
```

The sample version code is only an example. The tool cannot know which codes have already been used in your Play Console. Preflight fails before copying/cooking if any required signing input is missing or the certificate is wrong/debug-only.

The script copies source/config/plugin/content inputs into a temporary project, disables the development SVN junction startup script in that copy, and builds Android ASTC Shipping with `ForDistribution=True`, AAB enabled and the supplied upload key. The release copy enables GooglePAD and disables packaging data inside the base APK, so Unreal delivers the game's main OBB as the `obbassets` install-time asset pack. Ordinary development APK settings stay unchanged. Verify the final AAB contains `obbassets/assets/main.obb.png` and measure generated download sizes before upload; the game data should not inflate the base module beyond Play's limit. Credentials and generated Gradle files stay inside that temporary tree and are removed on normal completion or handled failure. Build console/log output is password-redacted. A forcibly killed process can leave a temporary directory; remove only its `P_RD-AndroidRelease-*` tree after confirming no build still uses it.

Both native build invocations disable adaptive unity. The disposable copy has no Git metadata, so Unreal would otherwise treat every writable source file as locally changed and unnecessarily split all unity translation units. Normal unity compilation remains enabled, matching a clean Git checkout.

Only after verification succeeds does the output directory receive the signed AAB, a signed universal APK generated from that AAB for testing, and `verification.json`. `build.log` is retained for both success and failure. No Play upload, release or device installation happens automatically.

The release checks cover:

- Pinned upload certificate and non-debug signer; valid AAB JAR/APK signatures.
- Final package name, target SDK 36+, no debuggable/test-only application.
- No unused Play `BILLING` permission: this game uses ads and has no in-app purchase flow. Android config explicitly disables Unreal's inherited IAP default; Google rejects an unversioned billing permission as legacy AIDL billing.
- AAB requests 16 KB APK page alignment; every included native ELF has compatible LOAD segments and GNU_RELRO.
- APK ZIP 16 KB alignment, including the universal APK produced from the actual AAB.

To validate an independently built artifact with the same certificate pin:

```powershell
python Tools/Android/verify_release.py path/to/P_RD.aab --bundletool path/to/bundletool.jar --report outputs/release-verification.json
python Tools/Android/verify_release.py path/to/P_RD.apk
```

These checks do not prove device execution, Play acceptance, asset completeness, performance, or account/policy compliance. Complete installation/update tests on a 16 KB device/emulator, launch-to-combat smoke tests, save/resume tests, and a Play internal-testing upload before public release. Test both cold and warm media caches. Capture the exact Git commit, SVN revision and verification report with the release.

## Beta entry-ad test

The Android package is `com.aurelight.mercenaryguildoftheruinedkingdom` for the first Play registration. Older local `com.AssortRock.P_RD` test installations are a different app and retain their own saves.

```powershell
python Tools/Android/build_release.py --version-code 3 --output outputs/android-beta-v3 --beta-entry-ads
```

This opt-in beta build uses Google Mobile Ads SDK 25.4.0 and Google's public demo app/interstitial IDs. Tapping New Start or Continue attempts one ready test ad per entry, then continues the original action after dismissal. No ad, load failure or display failure proceeds immediately; it never delays gameplay waiting for an ad or shows a late-loaded ad over gameplay. Repeated taps, duplicate Android callbacks and a closed title cannot trigger a second or stale entry. Returning to the title preloads the next entry's ad. Button captions disclose the ad in Korean and English.

The integration intentionally has no live ad-unit setting. Google Play's [disruptive ads policy](https://support.google.com/googleplay/android-developer/answer/9857753?hl=en) identifies unexpected full-screen ads after a start action and before its content as a prohibited placement. This is a demo beta flow, not a production monetization approval. Production monetization requires an appropriate placement, the owner's AdMob app/ad units, required consent handling and corresponding Play declarations. Default release builds omit this demo SDK; `verification.json` records whether the beta demo flag was used.

Validate `P_RD.Ads.EntryContinuation` and `P_RD.UI.Title.MenuRowsClickable`, then inspect a Google-labeled test ad and both entry paths on Android. Never click a live ad while testing. Native/SDK packaging and actual Play installation remain separate verification steps.

## Mobile memory and texture policy

`Config/Android/AndroidScalability.ini` sets texture streaming budgets of 256/384/512/768 MiB for quality tiers 0/1/2/3. These are starting budgets awaiting target-device measurements. The engine's Android device tiers choose an initial scalability level; user graphics settings can subsequently change the budget because it is not pinned at project/device-profile priority. Mobile Preview may deliberately suppress texture-pool changes; validate the effective value on Android.

`DefaultDeviceProfiles.ini` retains 1024 world/normal texture cook caps. UE ignores `MaxLODSize` for `NoMipmaps` UI textures, so a blanket UI group cap would not solve oversized UI allocations. Use targeted per-asset Android `Downscale` overrides for those textures and confirm actual cooked dimensions and legibility on the target display. Small text and icons should not be globally reduced. Non-streaming UI allocations are separate from the streaming pool. The eventual device/FPS targets must determine further reductions; no universal device performance guarantee is implied.

An optional reproducible trial is included:

```powershell
python Tools/Android/build_release.py --version-code 2 --output outputs/android-release-ui-trial-v2 --prepare-ui
```

The reward chest is a 6×6 animation atlas. Its 4092×2730 source contains 682×455 frames; the old Android Downscale of 5.328125 reduced each frame to about 128×85. `RewardTextureQuality` now restores the atlas's Android scale to 1 on editor/cooker load. Other platforms and other textures retain their existing values. The correction is applied in memory and does not modify the shared SVN file.

The optional preparation step still adjusts only the marked disposable project copy. The chest budget is now 4096 with a **minimum** of 4092×2730, while the map retains its 2048 maximum and any stronger existing downscale. A lower-resolution chest now fails cooked-size verification instead of passing merely because it is small.

```powershell
python Tools/Android/verify_ui_cooked_sizes.py path/to/android-listtextures.log
```

Use a packaged Android Development run with both textures loaded for this command. It requires the `Cooked/OnDisk:` header from `ListTextures`; editor dimensions alone are not cooked-size evidence. Shipping omits the diagnostic console. Check the actual chest animation visually on the device as well.

## References

- [Android App Bundles](https://developer.android.com/guide/app-bundle)
- [Android signing](https://developer.android.com/studio/publish/app-signing)
- [bundletool](https://developer.android.com/tools/bundletool)
- [16 KB page-size validation](https://developer.android.com/guide/practices/page-sizes)
