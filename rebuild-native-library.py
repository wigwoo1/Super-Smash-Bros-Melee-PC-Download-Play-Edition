"""Rebuild the diagnostic static library; this does not produce a playable game."""
import argparse
import os
from pathlib import Path
import subprocess
import shutil


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configure-only', action='store_true')
    parser.add_argument('--generated-includes', type=Path)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--build-type', choices=('Debug', 'Release', 'RelWithDebInfo'),
                        default='Debug', help='Must match the Aurora runtime build when linking')
    for tool in ('cmake', 'ninja', 'clang'):
        parser.add_argument('--' + tool, type=Path, help='Explicit tool executable path')
    args = parser.parse_args()
    resolved = {tool: str(getattr(args, tool).resolve()) if getattr(args, tool)
                else shutil.which(tool) for tool in ('cmake', 'ninja', 'clang')}
    missing = [tool for tool, path in resolved.items() if not path or not Path(path).is_file()]
    if missing:
        parser.error('Required tools missing from PATH: ' + ', '.join(missing))
    root = Path(__file__).resolve().parent
    packaged = (root / 'source' / 'CMakeLists.txt').is_file()
    source = root / ('source' if packaged else 'melee')
    aurora = root / ('dependencies/aurora' if packaged else 'aurora')
    if not (source / 'CMakeLists.txt').is_file() or not (aurora / 'include').is_dir():
        parser.error('Expected source and Aurora headers beside this script.')
    generated = args.generated_includes.resolve() if args.generated_includes else None
    if generated:
        for font in ('debug_font', 'sislib_font'):
            if not (generated / 'sysdolphin' / 'baselib' / (font + '.inc')).is_file():
                parser.error('Missing local font includes; see NATIVE_REBUILD.txt.')
    build = args.build_dir.resolve() if args.build_dir else root / 'build-native-library'
    env = os.environ.copy()
    # Avoid a developer-shell variable silently selecting another checkout.
    env.pop('AURORA_SRC', None)
    command = [resolved['cmake'], '-S', str(source), '-B', str(build), '-G', 'Ninja',
               '-DCMAKE_MAKE_PROGRAM=' + resolved['ninja'],
               '-DCMAKE_C_COMPILER=' + resolved['clang'],
               '-DCMAKE_C_COMPILER_TARGET=i686-pc-windows-msvc',
               '-DCMAKE_BUILD_TYPE=' + args.build_type, '-DAURORA_SRC=' + str(aurora),
               '-DMELEE_UNLOCK_ALL=ON', '-DMELEE_NO_SAVES=ON',
               '-DMELEE_GENERATED_INCLUDE_DIR=' + (str(generated) if generated else '')]
    print('Diagnostic static library only; no playable executable.', flush=True)
    subprocess.run(command, env=env, check=True)
    if not args.configure_only:
        subprocess.run([resolved['cmake'], '--build', str(build), '--parallel', '4'], env=env, check=True)


if __name__ == '__main__':
    main()
