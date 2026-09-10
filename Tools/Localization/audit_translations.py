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
for culture in ('en', 'ko'):
    raw = (root / culture / 'Game.archive').read_bytes()
    data = json.loads(raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig'))
    count = 0
    for namespace, item in entries(data):
        count += 1
        source, text = item['Source']['Text'], item['Translation']['Text']
        identity = f"{culture}:{namespace}:{item['Key']}"
        if source and not text:
            errors.append(identity + ': missing translation')
        if culture == 'en' and re.search('[가-힣]', text):
            errors.append(identity + ': Korean remains in English translation')
        if sorted(re.findall(r'\{[^{}]*\}', source)) != sorted(re.findall(r'\{[^{}]*\}', text)):
            errors.append(identity + ': format arguments differ')
    print(f'{culture}: checked {count} entries')
for error in errors:
    print(error)
sys.exit(bool(errors))
