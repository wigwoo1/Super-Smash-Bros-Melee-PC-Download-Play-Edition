"""Import the audited Melee HD archive into a NEW local content directory.

Requires the native parser audit JSON. Never changes original game files.
Unresolved conflicts remain excluded. Optional textures require --include-optional;
main artwork wins when optional items reuse the same original texture identity.
--include-mips copies validated half-size mip chains; incompatible chains stay excluded.
This local import tool does not establish permission to redistribute assets.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path, PurePosixPath
import zipfile

ARCHIVE_SHA256 = 'b67807009688036ea10fd7662d0f14525f0d0c95d7010a5bac4a85501564b4d5'


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def collect_mips(z, rows, selected):
    """Preserve authored pixels, including aliases only when base bytes match."""
    by_path = {r['path']: r for r in rows}
    by_key = {r['key']: r for r in selected}
    groups, rejected, alias_hashes = {}, [], {}
    for row in rows:
        match = re.search(r'_mip([1-9][0-9]*)\.png$', row['path'])
        if not match:
            continue
        base_path = row['path'][:match.start()] + '.png'
        base_row = by_path.get(base_path)
        base = by_key.get(base_row['native_key']) if base_row else None
        if base is None:
            rejected.append({'path': row['path'], 'reason': 'mip_base_not_selected'})
            continue
        if base_path != base['path']:
            if base_path not in alias_hashes:
                alias_hashes[base_path] = sha256(z.read(base_path))
            if alias_hashes[base_path] != base['sha256']:
                rejected.append({'path': row['path'], 'reason': 'mip_base_conflict'})
                continue
        level = int(match[1])
        output_name = PurePosixPath(base['path']).stem + f'_mip{level}.png'
        groups.setdefault(base['key'], []).append({**row, 'level': level,
            'base_path': base['path'], 'output_name': output_name})
    accepted = []
    for key, candidates in sorted(groups.items()):
        base = by_key[key]
        levels = sorted({r['level'] for r in candidates})
        reason = None
        if levels[-1] > min(31, max(base['width'], base['height']).bit_length() - 1):
            reason = 'mip_level_out_of_range'
        elif levels != list(range(1, levels[-1] + 1)):
            reason = 'mip_gap'
        elif any((r['width'], r['height']) !=
                 (max(1, base['width'] >> r['level']), max(1, base['height'] >> r['level']))
                 for r in candidates):
            reason = 'mip_dimensions'
        chain = []
        if reason is None:
            for level in levels:
                choices = []
                for r in candidates:
                    if r['level'] == level:
                        data = z.read(r['path'])
                        if len(data) != r['bytes']:
                            raise ValueError('Mip audit size mismatch: ' + r['path'])
                        choices.append({k: r[k] for k in ('path', 'bytes', 'width', 'height', 'level', 'base_path', 'output_name')})
                        choices[-1]['sha256'] = sha256(data)
                if len({r['sha256'] for r in choices}) != 1:
                    reason = 'mip_content_conflict'
                    break
                choices.sort(key=lambda r: r['path'])
                chain.append(choices[0])
        if reason:
            rejected.extend({'path': r['path'], 'base_path': base['path'], 'reason': reason}
                            for r in candidates)
        else:
            accepted.extend(chain)
    return accepted, rejected


def prepare(archive, audit, include_optional=False, include_mips=False):
    with archive.open('rb') as source:
        if hashlib.file_digest(source, 'sha256').hexdigest() != ARCHIVE_SHA256:
            raise ValueError('Archive differs from the inspected release; inventory it first.')
    rows = json.loads(audit.read_text(encoding='utf-8'))
    if rows.get('name') != 'textures' or len(rows.get('files', [])) != 5356:
        raise ValueError('Expected complete native-parser audit for the inspected texture pack.')
    groups = {}
    skipped = []
    with zipfile.ZipFile(archive) as z:
        if len({i.filename for i in z.infolist()}) != len(z.infolist()):
            raise ValueError('Duplicate archive paths.')
        for row in rows['files']:
            path = row['path']
            parts = PurePosixPath(path).parts
            if '\\' in path or ':' in path or '..' in parts or PurePosixPath(path).is_absolute():
                raise ValueError('Unsafe archive path: ' + path)
            key = row['native_key']
            allowed_optional = include_optional and path.startswith((
                'GALE01/optional/characters/', 'GALE01/optional/items/',
                'GALE01/optional/stages/casual/'))
            if not key or not (path.startswith('GALE01/main/') or allowed_optional):
                skipped.append({'path': path, 'reason': 'optional_or_unmapped'})
                continue
            data = z.read(path)  # ZIP CRC verified by zipfile.
            if len(data) != row['bytes']:
                raise ValueError('Audit size mismatch: ' + path)
            groups.setdefault(key, []).append({'path': path, 'sha256': sha256(data),
                'bytes': len(data), 'width': row['width'], 'height': row['height']})
        selected, conflicts = [], []
        for key, candidates in sorted(groups.items()):
            main = [r for r in candidates if r['path'].startswith('GALE01/main/')]
            if main and len(main) != len(candidates):
                skipped.extend({'path': r['path'], 'reason': 'main_precedence'}
                               for r in candidates if not r['path'].startswith('GALE01/main/'))
                candidates = main
            if len({r['sha256'] for r in candidates}) > 1:
                conflicts.append({'key': key, 'candidates': candidates})
                continue
            candidates.sort(key=lambda r: (r['path'].lower(), r['path']))
            selected.append({'key': key, **candidates[0]})
            skipped.extend({'path': r['path'], 'reason': 'identical_duplicate'} for r in candidates[1:])
        credits = z.read('GALE01/credits.txt').decode('utf-8')
        sidecars, mip_rejected = collect_mips(z, rows['files'], selected) if include_mips else ([], [])
    result = {'schema': 1, 'archive_sha256': ARCHIVE_SHA256,
            'policy': ('main plus optional characters/items/casual stages; main wins collisions; remaining conflicts excluded'
                       if include_optional else
                       'main only; identical duplicates collapsed; conflicts excluded pending visual review'),
            'runtime_verified': False, 'redistribution_permission': 'not established by this tool',
            'selected': selected, 'conflicts': conflicts, 'skipped': skipped, 'credits': credits}
    if include_mips:
        accepted_paths = {r['path'] for r in sidecars}
        result['skipped'] = [r for r in skipped if r['path'] not in accepted_paths]
        result['sidecars'] = sidecars
        result['mip_rejected'] = mip_rejected
    return result


def install(archive, destination, manifest):
    # A new directory avoids replacing originals, existing imports, or user mods.
    destination.mkdir(parents=False, exist_ok=False)
    content = destination / 'textures'
    content.mkdir()
    with zipfile.ZipFile(archive) as z:
        for entry in manifest['selected'] + manifest.get('sidecars', []):
            name = entry.get('output_name', PurePosixPath(entry['path']).name)
            if PurePosixPath(name).name != name or '\\' in name or ':' in name:
                raise ValueError('Unsafe output filename')
            target = content / name
            data = z.read(entry['path'])
            if sha256(data) != entry['sha256']:
                raise ValueError('Archive changed during import.')
            with target.open('xb') as output:
                output.write(data)
    (destination / 'credits.txt').write_bytes(manifest['credits'].encode('utf-8'))
    # Manifest is written last; absence means the import did not complete.
    (destination / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf-8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('audit', type=Path)
    parser.add_argument('destination', type=Path)
    parser.add_argument('--plan-only', action='store_true', help='write manifest only, no images')
    parser.add_argument('--include-optional', action='store_true',
                        help='include character, item and casual-stage base textures; main artwork wins shared keys')
    parser.add_argument('--include-mips', action='store_true', help='preserve compatible authored mip chains')
    args = parser.parse_args()
    manifest = prepare(args.archive, args.audit, args.include_optional, args.include_mips)
    if args.plan_only:
        with args.destination.open('x', encoding='utf-8') as output:
            json.dump(manifest, output, indent=2)
    else:
        install(args.archive, args.destination, manifest)
    print(json.dumps({'selected': len(manifest['selected']), 'conflicts': len(manifest['conflicts']),
                      'sidecars': len(manifest.get('sidecars', [])),
                      'skipped': len(manifest['skipped']), 'installed': not args.plan_only}))


if __name__ == '__main__':
    main()
