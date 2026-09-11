import io
import os
from pathlib import Path
import struct
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build_release import redact, signing_environment
from verify_release import VerificationError, normalize_fingerprint, verify_elf, verify_manifest
from ui_policy import TARGETS, android_downscale
from verify_ui_cooked_sizes import verify_log


def elf(alignment=16384, relro=True, offset=0, virtual=0):
    header = bytearray(64)
    header[:6] = b'\x7fELF\x02\x01'
    struct.pack_into('<Q', header, 32, 64)
    struct.pack_into('<HH', header, 54, 56, 2 if relro else 1)
    load = struct.pack('<IIQQQQQQ', 1, 5, offset, virtual, 0, 64, 64, alignment)
    return io.BytesIO(header + load + (struct.pack('<IIQQQQQQ', 0x6474E552, 4, 0, 0, 0, 0, 0, 1) if relro else b''))


class ReleaseValidationTests(unittest.TestCase):
    def test_all_16k_load_segments_and_relro_are_accepted(self):
        self.assertTrue(verify_elf(elf(), 'lib/arm64-v8a/test.so')['relro'])

    def test_4k_prebuilt_library_is_rejected(self):
        with self.assertRaisesRegex(VerificationError, '16 KB'):
            verify_elf(elf(4096), 'lib/arm64-v8a/old-sdk.so')

    def test_misaligned_load_address_is_rejected(self):
        with self.assertRaisesRegex(VerificationError, '16 KB'):
            verify_elf(elf(offset=4096), 'lib/arm64-v8a/test.so')

    def test_missing_relro_is_rejected(self):
        with self.assertRaisesRegex(VerificationError, 'GNU_RELRO'):
            verify_elf(elf(relro=False), 'lib/arm64-v8a/test.so')

    def test_truncated_or_non_elf_library_is_rejected(self):
        with self.assertRaises(VerificationError):
            verify_elf(io.BytesIO(b'not a library'), 'broken.so')

    def test_debug_or_test_only_manifest_is_rejected(self):
        template = '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.AssortRock.P_RD"><uses-sdk android:targetSdkVersion="36"/><application %s/></manifest>'
        for flag in ('android:debuggable="true"', 'android:testOnly="true"'):
            with self.subTest(flag=flag), self.assertRaises(VerificationError):
                verify_manifest(template % flag, 'com.AssortRock.P_RD', 36)
        self.assertFalse(verify_manifest(template % '', 'com.AssortRock.P_RD', 36)['debuggable'])

    def test_wrong_package_or_old_target_is_rejected(self):
        xml = '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="other"><uses-sdk android:targetSdkVersion="35"/><application/></manifest>'
        with self.assertRaises(VerificationError):
            verify_manifest(xml, 'com.AssortRock.P_RD', 36)
        with self.assertRaises(VerificationError):
            verify_manifest(xml, 'other', 36)

    def test_unused_billing_permission_blocks_release(self):
        xml = '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="com.aurelight.mercenaryguildoftheruinedkingdom"><uses-sdk android:targetSdkVersion="36"/><uses-permission android:name="com.android.vending.BILLING"/><application/></manifest>'
        with self.assertRaisesRegex(VerificationError, 'unused Play BILLING'):
            verify_manifest(xml, 'com.aurelight.mercenaryguildoftheruinedkingdom', 36)

    def test_ad_free_release_rejects_engine_admob_default_and_sdk_permissions(self):
        template = '<manifest xmlns:android="http://schemas.android.com/apk/res/android" package="game"><uses-sdk android:targetSdkVersion="36"/>%s<application>%s</application></manifest>'
        cases = (
            ('', '<activity android:name="com.google.android.gms.ads.AdActivity"/>'),
            ('', '<meta-data android:name="com.google.android.gms.ads.APPLICATION_ID" android:value="demo"/>'),
            ('<uses-permission android:name="com.google.android.gms.permission.AD_ID"/>', ''),
            ('<uses-permission android:name="android.permission.ACCESS_ADSERVICES_AD_ID"/>', ''),
        )
        for permission, component in cases:
            with self.subTest(component=component, permission=permission), self.assertRaisesRegex(VerificationError, 'ad-free'):
                verify_manifest(template % (permission, component), 'game', 36)

    def test_missing_signing_credentials_stop_before_build(self):
        with patch.dict(os.environ, {}, clear=True), self.assertRaisesRegex(VerificationError, 'signing is unavailable'):
            signing_environment()

    def test_signing_passwords_are_redacted_from_output(self):
        signing = {'RD_ANDROID_UPLOAD_STORE_PASSWORD': 'store-secret', 'RD_ANDROID_UPLOAD_KEY_PASSWORD': 'key-secret'}
        self.assertEqual(redact('store-secret/key-secret', signing), '[redacted]/[redacted]')

    def test_certificate_pin_is_required(self):
        with self.assertRaises(VerificationError):
            normalize_fingerprint('')
        self.assertEqual(normalize_fingerprint(':'.join(['AB'] * 32)), 'AB' * 32)

    def test_ui_trial_preserves_existing_stronger_android_limit(self):
        self.assertEqual(android_downscale(4092, 2730, 2048, 6.0), 6.0)
        self.assertEqual(android_downscale(1024, 3072, 2048, 0), 1.5)

    def test_editor_dimensions_do_not_count_as_cooked_ui_verification(self):
        with self.assertRaisesRegex(ValueError, 'packaged Android'):
            verify_log('MaxAllowedSize: Width x Height')

    def test_cooked_ui_requires_both_assets_within_budget(self):
        lines = ['Cooked/OnDisk: Width x Height']
        for asset in TARGETS:
            size = '4092x2730' if 'ChestTripleBurst' in asset else '2048x1368'
            lines.append(f'{size} (11000 KB, ?), {size} (11000 KB), PF_ASTC_4x4, TEXTUREGROUP_UI, {asset}, NO')
        self.assertTrue(verify_log('\n'.join(lines))['passed'])
        with self.assertRaisesRegex(ValueError, 'exceeds'):
            verify_log('\n'.join(lines).replace('2048x1368', '4092x2730'))
        with self.assertRaisesRegex(ValueError, 'missing'):
            verify_log('\n'.join(lines[:-1]))
        with self.assertRaisesRegex(ValueError, 'below required frame resolution'):
            verify_log('\n'.join(lines).replace('4092x2730', '768x512'))


if __name__ == '__main__':
    unittest.main()
