"""Fail-closed, read-only checks for an Android release APK or AAB (Python 3.11+)."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import ssl
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
import zipfile


class VerificationError(RuntimeError):
    pass


def run_tool(args: list[str | Path]) -> str:
    result = subprocess.run([str(arg) for arg in args], capture_output=True, text=True,
                            encoding="utf-8", errors="replace")
    if result.returncode:
        # Tool diagnostics can contain local signing settings; do not echo them.
        raise VerificationError(f"{Path(args[0]).name} failed with exit code {result.returncode}")
    return result.stdout + result.stderr


def normalize_fingerprint(value: str) -> str:
    value = value.replace(":", "").strip().upper()
    if not re.fullmatch(r"[0-9A-F]{64}", value):
        raise VerificationError("An explicit SHA-256 upload certificate fingerprint is required")
    return value


def android_tool(sdk: Path, name: str) -> Path:
    versions = sorted((sdk / "build-tools").glob("*"), reverse=True,
                      key=lambda p: tuple(int(n) for n in re.findall(r"\d+", p.name)))
    suffix = ".bat" if name == "apksigner" and os.name == "nt" else ".exe" if os.name == "nt" else ""
    for version in versions:
        if int(version.name.split(".")[0]) >= 35 and (version / (name + suffix)).is_file():
            return version / (name + suffix)
    raise VerificationError(f"Android SDK build-tools 35+ with {name} are required")


def java_tool(java_home: Path, name: str) -> Path:
    path = java_home / "bin" / (name + (".exe" if os.name == "nt" else ""))
    if not path.is_file():
        raise VerificationError(f"A JDK with {name} is required")
    return path


def verify_elf(stream, name: str) -> dict:
    header = stream.read(64)
    if len(header) != 64 or header[:6] != b"\x7fELF\x02\x01":
        raise VerificationError(f"{name}: expected a little-endian 64-bit ELF library")
    phoff, = struct.unpack_from("<Q", header, 32)
    phsize, phcount = struct.unpack_from("<HH", header, 54)
    if phoff < 64 or phoff > 16 * 1024 * 1024 or phsize < 56 or not 0 < phcount < 1024:
        raise VerificationError(f"{name}: invalid ELF program header table")
    stream.seek(phoff)
    loads = 0
    relro = False
    for _ in range(phcount):
        ph = stream.read(phsize)
        if len(ph) != phsize:
            raise VerificationError(f"{name}: truncated ELF program header table")
        kind, = struct.unpack_from("<I", ph)
        if kind == 1:  # PT_LOAD
            offset, virtual = struct.unpack_from("<QQ", ph, 8)
            alignment, = struct.unpack_from("<Q", ph, 48)
            if alignment < 16384 or alignment & (alignment - 1) or (virtual - offset) % 16384:
                raise VerificationError(f"{name}: a LOAD segment is not 16 KB compatible")
            loads += 1
        relro |= kind == 0x6474E552  # PT_GNU_RELRO
    if not loads or not relro:
        raise VerificationError(f"{name}: missing LOAD segments or GNU_RELRO")
    return {"name": name, "load_segments": loads, "minimum_page_alignment": 16384, "relro": True}


def verify_native_libraries(artifact: Path) -> list[dict]:
    libraries = []
    with zipfile.ZipFile(artifact) as archive:
        for entry in archive.infolist():
            if re.search(r"(?:^|/)lib/[^/]+/[^/]+\.so$", entry.filename):
                with archive.open(entry) as stream:
                    libraries.append(verify_elf(stream, entry.filename))
    if not any("/arm64-v8a/" in item["name"] for item in libraries):
        raise VerificationError("Release artifact contains no arm64-v8a native libraries")
    return libraries


def verify_manifest(xml: str, package: str, min_target: int) -> dict:
    root = ET.fromstring(xml)
    ns = "{http://schemas.android.com/apk/res/android}"
    if root.get("package") != package:
        raise VerificationError("Manifest package name does not match the release package")
    app = root.find("application")
    if app is None or app.get(ns + "debuggable", "false").lower() not in ("false", "0"):
        raise VerificationError("Release application is debuggable or has no application node")
    if app.get(ns + "testOnly", "false").lower() not in ("false", "0"):
        raise VerificationError("Release application is marked testOnly")
    sdk = root.find("uses-sdk")
    target = int(sdk.get(ns + "targetSdkVersion", "0")) if sdk is not None else 0
    if target < min_target:
        raise VerificationError(f"Manifest target SDK must be at least {min_target}")
    return {"package": package, "target_sdk": target, "debuggable": False, "test_only": False}


def verify_artifact(artifact: Path, sdk: Path, java_home: Path, bundletool: Path | None,
                    fingerprint: str, package: str, min_target: int) -> dict:
    fingerprint = normalize_fingerprint(fingerprint)
    if not artifact.is_file():
        raise VerificationError("Release artifact does not exist")
    if artifact.suffix.lower() == ".apk":
        badging = run_tool([android_tool(sdk, "aapt"), "dump", "badging", artifact])
        if "application-debuggable" in badging or "application-testOnly" in badging:
            raise VerificationError("APK is debuggable or test-only")
        manifest_tree = run_tool([android_tool(sdk, "aapt"), "dump", "xmltree", artifact, "AndroidManifest.xml"])
        if re.search(r"android:testOnly[^\n]*\(type 0x12\)0xffffffff", manifest_tree, re.I):
            raise VerificationError("APK is test-only")
        if not re.search(r"^package: name='" + re.escape(package) + "'", badging, re.M):
            raise VerificationError("APK package name does not match")
        match = re.search(r"^targetSdkVersion:'(\d+)'", badging, re.M)
        if not match or int(match[1]) < min_target:
            raise VerificationError(f"APK target SDK must be at least {min_target}")
        signing = run_tool([android_tool(sdk, "apksigner"), "verify", "--verbose", "--print-certs", artifact])
        certs = re.findall(r"Signer #\d+ certificate SHA-256 digest:\s*([0-9a-fA-F]+)", signing)
        if not certs or {value.upper() for value in certs} != {fingerprint}:
            raise VerificationError("APK signer does not match the expected upload certificate")
        if re.search(r"certificate DN:.*CN=Android Debug", signing, re.I):
            raise VerificationError("Android Debug signing certificates are not release certificates")
        run_tool([android_tool(sdk, "zipalign"), "-c", "-P", "16", "4", artifact])
        manifest = {"package": package, "target_sdk": int(match[1]), "debuggable": False}
    elif artifact.suffix.lower() == ".aab":
        if bundletool is None or not bundletool.is_file():
            raise VerificationError("A verified local bundletool JAR is required for AAB checks")
        java = java_tool(java_home, "java")
        base = [java, "-jar", bundletool]
        run_tool(base + ["validate", f"--bundle={artifact}"])
        manifest = verify_manifest(run_tool(base + ["dump", "manifest", f"--bundle={artifact}", "--module=base"]),
                                   package, min_target)
        config = run_tool(base + ["dump", "config", f"--bundle={artifact}"])
        if "PAGE_ALIGNMENT_16K" not in config:
            raise VerificationError("AAB does not request PAGE_ALIGNMENT_16K for generated APKs")
        signing = run_tool([java_tool(java_home, "jarsigner"), "-J-Duser.language=en", "-J-Duser.country=US",
                            "-verify", "-verbose", "-certs", artifact])
        if "jar verified." not in signing or re.search(r"unsigned entries|jar is unsigned", signing, re.I):
            raise VerificationError("AAB JAR signature is invalid or contains unsigned entries")
        pem = run_tool([java_tool(java_home, "keytool"), "-printcert", "-jarfile", artifact, "-rfc"])
        cert = re.search(r"-----BEGIN CERTIFICATE-----.*?-----END CERTIFICATE-----", pem, re.S)
        if cert is None or hashlib.sha256(ssl.PEM_cert_to_DER_cert(cert[0])).hexdigest().upper() != fingerprint:
            raise VerificationError("AAB signer does not match the expected upload certificate")
        if re.search(r"CN=Android Debug", signing, re.I):
            raise VerificationError("Android Debug signing certificates are not release certificates")
    else:
        raise VerificationError("Expected an .apk or .aab artifact")
    with artifact.open("rb") as handle:
        digest = hashlib.file_digest(handle, "sha256").hexdigest()
    return {"artifact": artifact.name, "sha256": digest,
            "bytes": artifact.stat().st_size, "manifest": manifest, "certificate_sha256": fingerprint,
            "native_libraries": verify_native_libraries(artifact), "passed": True}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=Path)
    parser.add_argument("--sdk", type=Path, default=os.environ.get("ANDROID_HOME") or os.environ.get("ANDROID_SDK_ROOT"))
    parser.add_argument("--java-home", type=Path, default=os.environ.get("JAVA_HOME"))
    parser.add_argument("--bundletool", type=Path)
    parser.add_argument("--expected-cert-sha256", default=os.environ.get("RD_ANDROID_UPLOAD_CERT_SHA256", ""))
    parser.add_argument("--package", default="com.aurelight.mercenaryguildoftheruinedkingdom")
    parser.add_argument("--min-target-sdk", type=int, default=36)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.sdk is None or args.java_home is None:
        raise VerificationError("ANDROID_HOME and JAVA_HOME (or explicit paths) are required")
    report = verify_artifact(args.artifact, args.sdk, args.java_home, args.bundletool,
                             args.expected_cert_sha256, args.package, args.min_target_sdk)
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    try:
        main()
    except (VerificationError, ValueError, ET.ParseError, OSError, zipfile.BadZipFile) as error:
        print(f"Release verification failed: {error}", file=sys.stderr)
        sys.exit(1)
