"""Build and verify a signed Android AAB in an isolated project copy; never upload it."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile

from verify_release import (VerificationError, android_tool, java_tool,
                            normalize_fingerprint, run_tool, verify_artifact)


SIGNING_VARIABLES = ("RD_ANDROID_UPLOAD_KEYSTORE", "RD_ANDROID_UPLOAD_ALIAS",
                     "RD_ANDROID_UPLOAD_STORE_PASSWORD", "RD_ANDROID_UPLOAD_CERT_SHA256")


def signing_environment() -> dict[str, str]:
    missing = [name for name in SIGNING_VARIABLES if not os.environ.get(name)]
    if missing:
        raise VerificationError("Release signing is unavailable. Set external environment variables: " + ", ".join(missing))
    signing = {name: os.environ[name] for name in SIGNING_VARIABLES}
    signing["RD_ANDROID_UPLOAD_KEY_PASSWORD"] = os.environ.get("RD_ANDROID_UPLOAD_KEY_PASSWORD") or signing["RD_ANDROID_UPLOAD_STORE_PASSWORD"]
    normalize_fingerprint(signing["RD_ANDROID_UPLOAD_CERT_SHA256"])
    key = Path(signing["RD_ANDROID_UPLOAD_KEYSTORE"]).resolve(strict=True)
    if not key.is_file():
        raise VerificationError("The external upload keystore must be a file")
    signing["RD_ANDROID_UPLOAD_KEYSTORE"] = str(key)
    # UE writes these into single-quoted Gradle strings. Reject unsupported inputs
    # before creating a project copy instead of leaking malformed Gradle lines.
    for name, value in signing.items():
        if name.endswith("PASSWORD") or name.endswith("ALIAS"):
            if any(char in value for char in "\r\n'\\\""):
                raise VerificationError(f"{name} contains characters unsupported by UE's generated signing configuration")
    if re.search(r"androiddebugkey", signing["RD_ANDROID_UPLOAD_ALIAS"], re.I):
        raise VerificationError("The Android debug key cannot be used for release")
    return signing


def redact(text: str, signing: dict[str, str]) -> str:
    passwords = {value for name, value in signing.items() if name.endswith("PASSWORD") and value}
    for value in sorted(passwords, key=len, reverse=True):
        text = text.replace(value, "[redacted]")
    return text


def append_ini(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8", newline="\n") as handle:
        handle.write("\n" + value + "\n")


def run_logged(arguments: list[str], env: dict[str, str], signing: dict[str, str], log) -> None:
    process = subprocess.Popen(arguments, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                               text=True, encoding="utf-8", errors="replace", env=env)
    try:
        for line in process.stdout:
            safe_line = redact(line, signing)
            log.write(safe_line)
            print(safe_line, end="", flush=True)
        if process.wait():
            raise VerificationError("Android release step failed; see the redacted build.log")
    finally:
        if process.poll() is None:
            # UAT owns compiler/cooker children which keep DLLs and signing files open.
            # Stop this build's process tree before the temporary copy is removed.
            subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            process.wait()


def copy_build_inputs(project: Path, destination: Path) -> Path:
    # Copy rather than junction mutable inputs. Cooker writes stay in this copy;
    # no credential or distribution setting is written to the source project.
    ignore = shutil.ignore_patterns(".git", ".svn", "Binaries", "Intermediate", "DerivedDataCache",
                                    "Saved", "*.keystore", "*.jks", "*.p12", "*.pfx")
    for directory in ("Source", "Config", "Plugins", "Content", "Build"):
        source = project.parent / directory
        if source.is_dir():
            shutil.copytree(source, destination / directory, ignore=ignore)
    copied_project = destination / project.name
    shutil.copy2(project, copied_project)
    (destination / ".android-release-copy.json").write_text(json.dumps({
        "copy_root": str(destination.resolve()), "source_root": str(project.parent.resolve())
    }), encoding="utf-8")
    # The snapshot already contains SVN assets. Do not run the developer's
    # startup junction/hook installer from a release cook.
    append_ini(destination / "Config" / "DefaultEngine.ini", """[/Script/PythonScriptPlugin.PythonScriptPluginSettings]
!StartupScripts=ClearArray
[/Script/AndroidFileServerEditor.AndroidFileServerRuntimeSettings]
bEnablePlugin=False
bIncludeInShipping=False
bAllowExternalStartInShipping=False
""")
    return copied_project


def configure_release(project: Path, signing: dict[str, str], version: int) -> None:
    # Keep the only copied key and plaintext settings inside the disposable build
    # directory; the directory is removed in finally on success and failure.
    keystore = project.parent / "Build" / "Android" / "upload.keystore"
    keystore.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(signing["RD_ANDROID_UPLOAD_KEYSTORE"], keystore)
    append_ini(project.parent / "Config" / "Android" / "AndroidEngine.ini", f"""[/Script/AndroidRuntimeSettings.AndroidRuntimeSettings]
bEnableBundle=True
bEnableUniversalAPK=False
bBuildForArm64=True
bBuildForX8664=False
bPackageDataInsideApk=False
bAllowPatchOBBFile=False
StoreVersion={version}
KeyStore=upload.keystore
KeyAlias="{signing['RD_ANDROID_UPLOAD_ALIAS']}"
KeyStorePassword="{signing['RD_ANDROID_UPLOAD_STORE_PASSWORD']}"
KeyPassword="{signing['RD_ANDROID_UPLOAD_KEY_PASSWORD']}"
[/Script/GooglePADEditor.GooglePADRuntimeSettings]
bEnablePlugin=True
bOnlyDistribution=True
""")
    append_ini(project.parent / "Config" / "DefaultGame.ini", """[/Script/UnrealEd.ProjectPackagingSettings]
BuildConfiguration=PPBC_Shipping
ForDistribution=True
bUsePakFile=True
bUseIoStore=True
bCompressed=True
+IniKeyDenylist=KeyStore
+IniKeyDenylist=KeyAlias
+IniKeyDenylist=KeyStorePassword
+IniKeyDenylist=KeyPassword
+IniKeyDenylist=SecurityToken
""")


def preflight(project: Path, engine: Path, sdk: Path, java_home: Path,
              bundletool: Path, signing: dict[str, str]) -> None:
    if not project.is_file() or not (engine / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat").is_file():
        raise VerificationError("A Windows UE installation and a .uproject are required")
    if not (project.parent / "Content" / "SVN" / "MainLevel" / "Intro" / "L_Intro.umap").is_file():
        raise VerificationError("The project's external Content/SVN assets must be present before release cooking")
    if not bundletool.is_file():
        raise VerificationError("A verified local bundletool JAR is required")
    for tool in ("aapt", "apksigner", "zipalign"):
        android_tool(sdk, tool)
    for tool in ("java", "keytool", "jarsigner"):
        java_tool(java_home, tool)
    with tempfile.TemporaryDirectory(prefix="android-signing-check-") as temp:
        cert = Path(temp) / "upload.der"
        run_tool([java_tool(java_home, "keytool"), "-exportcert", "-keystore", signing["RD_ANDROID_UPLOAD_KEYSTORE"],
                  "-alias", signing["RD_ANDROID_UPLOAD_ALIAS"], "-storepass:env", "RD_ANDROID_UPLOAD_STORE_PASSWORD",
                  "-file", cert])
        if hashlib.sha256(cert.read_bytes()).hexdigest().upper() != normalize_fingerprint(signing["RD_ANDROID_UPLOAD_CERT_SHA256"]):
            raise VerificationError("Upload key certificate does not match RD_ANDROID_UPLOAD_CERT_SHA256")
        description = run_tool([java_tool(java_home, "keytool"), "-J-Duser.language=en", "-J-Duser.country=US",
                                "-list", "-v", "-keystore", signing["RD_ANDROID_UPLOAD_KEYSTORE"],
                                "-alias", signing["RD_ANDROID_UPLOAD_ALIAS"], "-storepass:env", "RD_ANDROID_UPLOAD_STORE_PASSWORD"])
        if "PrivateKeyEntry" not in description or re.search(r"CN=Android Debug", description, re.I):
            raise VerificationError("Release signing requires a non-debug private key entry")
        # Validate the private-key password before the expensive copy/cook. Only
        # this disposable probe is signed; no new key or published artifact is made.
        probe = Path(temp) / "signing-probe.jar"
        with zipfile.ZipFile(probe, "w") as archive:
            archive.writestr("probe.txt", "Android release signing preflight")
        key_password_variable = "RD_ANDROID_UPLOAD_KEY_PASSWORD" if os.environ.get("RD_ANDROID_UPLOAD_KEY_PASSWORD") else "RD_ANDROID_UPLOAD_STORE_PASSWORD"
        run_tool([java_tool(java_home, "jarsigner"), "-keystore", signing["RD_ANDROID_UPLOAD_KEYSTORE"],
                  "-storepass:env", "RD_ANDROID_UPLOAD_STORE_PASSWORD", "-keypass:env", key_password_variable,
                  probe, signing["RD_ANDROID_UPLOAD_ALIAS"]])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--project", type=Path, default=Path(__file__).resolve().parents[2] / "P_RD.uproject")
    parser.add_argument("--engine", type=Path, default=os.environ.get("UE_ROOT"))
    parser.add_argument("--sdk", type=Path, default=os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT"))
    parser.add_argument("--java-home", type=Path, default=os.environ.get("JAVA_HOME"))
    parser.add_argument("--bundletool", type=Path)
    parser.add_argument("--version-code", type=int, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--preflight-only", action="store_true")
    parser.add_argument("--prepare-ui", action="store_true", help="Apply the two targeted NoMipmaps UI trials only in the isolated copy")
    args = parser.parse_args()
    signing = signing_environment()  # Fail before build/cook if production credentials are unavailable.
    if os.name != "nt" or args.engine is None or args.sdk is None or args.java_home is None:
        raise VerificationError("Windows, UE_ROOT, ANDROID_HOME and JAVA_HOME are required")
    if not 0 < args.version_code < 2100000000:
        raise VerificationError("Use a positive, previously unused Android version code below 2100000000")
    args.project = args.project.resolve(strict=True)
    args.bundletool = args.bundletool or args.engine / "Engine/Build/Android/Prebuilt/bundletool/bundletool-all-1.18.1.jar"
    preflight(args.project, args.engine, args.sdk, args.java_home, args.bundletool, signing)
    if args.preflight_only:
        print("Release preflight passed. No build or upload was performed.")
        return
    if args.output is None:
        raise VerificationError("--output must name a new release output directory")
    output = args.output.resolve()
    if output.exists():
        raise VerificationError("Release output directory must not already exist")
    output.mkdir(parents=True)
    # tempfile creates a unique private build tree; all destructive cleanup is
    # restricted to this directory. Source project and external key are untouched.
    with tempfile.TemporaryDirectory(prefix="P_RD-AndroidRelease-") as temp:
        isolated = Path(temp)
        print("Copying project inputs for an isolated Android release cook...")
        project = copy_build_inputs(args.project, isolated / "Project")
        configure_release(project, signing, args.version_code)
        archive = isolated / "Archive"
        arguments = [str(args.engine / "Engine/Build/BatchFiles/RunUAT.bat"), "BuildCookRun",
                     f"-project={project}", "-noP4", "-platform=Android", "-cookflavor=ASTC",
                     "-nocompileeditor",
                     "-UbtArgs=-DisableAdaptiveUnity",
                     "-clientconfig=Shipping", "-build", "-cook", "-stage", "-pak", "-iostore", "-compressed",
                     "-package", "-distribution", "-archive", f"-archivedirectory={archive}", "-utf8output", "-unattended"]
        env = os.environ.copy()
        env["ANDROID_HOME"] = str(args.sdk)
        env["JAVA_HOME"] = str(args.java_home)
        with (output / "build.log").open("w", encoding="utf-8") as log:
            run_logged([str(args.engine / "Engine/Build/BatchFiles/Build.bat"), "P_RDEditor", "Win64", "Development",
                        f"-Project={project}", "-WaitMutex", "-NoHotReload", "-DisableAdaptiveUnity"], env, signing, log)
            if args.prepare_ui:
                run_logged([str(args.engine / "Engine/Binaries/Win64/UnrealEditor-Cmd.exe"), str(project),
                            "-run=pythonscript", f"-script={Path(__file__).resolve().parent / 'prepare_release_ui.py'}",
                            "-unattended", "-NullRHI", "-nosplash", "-stdout", "-FullStdOutLogOutput", "-UTF8Output"], env, signing, log)
                preparation = project.parent / "Saved/AndroidRelease/ui-preparation.json"
                if not preparation.is_file():
                    raise VerificationError("UI preparation did not produce its success report")
                shutil.copy2(preparation, output / preparation.name)
            run_logged(arguments, env, signing, log)
        bundles = list(archive.rglob("*.aab"))
        if len(bundles) != 1:
            raise VerificationError("Release build must archive exactly one signed AAB")
        bundle = bundles[0]
        reports = [verify_artifact(bundle, args.sdk, args.java_home, args.bundletool,
                                   signing["RD_ANDROID_UPLOAD_CERT_SHA256"], "com.aurelight.mercenaryguildoftheruinedkingdom", 36)]
        store_password = isolated / "store-password.txt"
        key_password = isolated / "key-password.txt"
        store_password.write_text(signing["RD_ANDROID_UPLOAD_STORE_PASSWORD"], encoding="utf-8")
        key_password.write_text(signing["RD_ANDROID_UPLOAD_KEY_PASSWORD"], encoding="utf-8")
        apks = isolated / "release.apks"
        run_tool([java_tool(args.java_home, "java"), "-jar", args.bundletool, "build-apks",
                  f"--bundle={bundle}", f"--output={apks}", "--mode=universal",
                  f"--ks={signing['RD_ANDROID_UPLOAD_KEYSTORE']}", f"--ks-key-alias={signing['RD_ANDROID_UPLOAD_ALIAS']}",
                  f"--ks-pass=file:{store_password}", f"--key-pass=file:{key_password}"])
        apk = isolated / "P_RD-release-universal.apk"
        with zipfile.ZipFile(apks) as archive_file, archive_file.open("universal.apk") as source, apk.open("wb") as target:
            shutil.copyfileobj(source, target)
        reports.append(verify_artifact(apk, args.sdk, args.java_home, args.bundletool,
                                       signing["RD_ANDROID_UPLOAD_CERT_SHA256"], "com.aurelight.mercenaryguildoftheruinedkingdom", 36))
        for report in reports:
            report["advertising_enabled"] = False
        # Only verified public artifacts and redacted output leave the private tree.
        shutil.copy2(bundle, output / bundle.name)
        shutil.copy2(apk, output / apk.name)
        (output / "verification.json").write_text(json.dumps(reports, indent=2) + "\n", encoding="utf-8")
    print("Release AAB and universal test APK passed signature, manifest and 16 KB checks. No upload was performed.")


if __name__ == "__main__":
    # UAT may emit replacement characters that the Windows cp949 console cannot encode.
    # Keep streaming the build instead of killing UAT and leaving locked temporary files.
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    sys.stderr.reconfigure(encoding="utf-8", errors="replace")
    try:
        main()
    except (VerificationError, ValueError, OSError, ET.ParseError, zipfile.BadZipFile) as error:
        # Do not dump tracebacks or captured tool output containing signing inputs.
        print(f"Android release stopped: {error}", file=sys.stderr)
        sys.exit(1)
