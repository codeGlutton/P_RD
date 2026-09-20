"""Check gathered Game archives before compiling/cooking localization resources."""
import json
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[2] / 'Content/Localization/Game'

def entries(node, namespace=''):
    namespace = node.get('Namespace', namespace)
    for child in node.get('Children', []):
        yield namespace, child
    for child in node.get('Subnamespaces', []):
        yield from entries(child, namespace)

errors = []

def load_catalog(path):
    raw = path.read_bytes()
    return json.loads(raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig'))

manifest_sources = {
    (namespace, key['Key']): item['Source']['Text']
    for namespace, item in entries(load_catalog(root / 'Game.manifest'))
    for key in item['Keys']
}

for culture in ('en', 'ko'):
    data = load_catalog(root / culture / 'Game.archive')
    count = 0
    seen = set()
    for namespace, item in entries(data):
        count += 1
        source, text = item['Source']['Text'], item['Translation']['Text']
        identity = f"{culture}:{namespace}:{item['Key']}"
        catalog_key = (namespace, item['Key'])
        seen.add(catalog_key)
        # Only the native archive can be compared directly with the manifest.
        # Other cultures may use a translated native source instead.
        if culture == 'en' and catalog_key in manifest_sources and source != manifest_sources[catalog_key]:
            errors.append(identity + ': source differs from the gathered manifest')
        if source and not text:
            errors.append(identity + ': missing translation')
        if culture == 'en' and re.search('[가-힣]', text):
            errors.append(identity + ': Korean remains in English translation')
        if sorted(re.findall(r'\{[^{}]*\}', source)) != sorted(re.findall(r'\{[^{}]*\}', text)):
            errors.append(identity + ': format arguments differ')
    print(f'{culture}: checked {count} entries')
    for namespace, key in sorted(manifest_sources.keys() - seen):
        errors.append(f'{culture}:{namespace}:{key}: missing archive entry')
for error in errors:
    print(error)
sys.exit(bool(errors))
