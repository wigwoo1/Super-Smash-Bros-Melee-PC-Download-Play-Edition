"""Configure/build the x86 native renderer runtime; does not link the game."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--configure-only', action='store_true')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--source', type=Path)
    parser.add_argument('--dependency-source-map', type=Path,
                        help='Optional JSON map of FetchContent names to existing source folders; no binary reuse')
    parser.add_argument('--jobs', type=int, default=4)
    for tool in ('cmake', 'ninja', 'clang', 'clangxx', 'cargo', 'rustc'):
        parser.add_argument('--' + tool, type=Path)
    args = parser.parse_args()
    if args.jobs < 1:
        parser.error('--jobs must be positive')
    root = Path(__file__).resolve().parent
    source = (args.source or (root / 'dependencies/aurora' if (root / 'dependencies/aurora').is_dir()
                              else root / 'aurora')).resolve()
    build = (args.build_dir or root / 'build-native-runtime-release').resolve()
    if not (source / 'CMakeLists.txt').is_file():
        parser.error('Aurora source directory is missing')
    target_runner = root / 'execute-target-for-cmake.py'
    if os.name != 'nt' or not target_runner.is_file():
        parser.error('This x86 Windows recipe requires Windows and execute-target-for-cmake.py beside it')
    if source == build:
        parser.error('Build output must be separate from source')
    resolved = {}
    for tool in ('cmake', 'ninja', 'clang', 'clangxx', 'cargo', 'rustc'):
        value = getattr(args, tool)
        executable = str(value.resolve()) if value else shutil.which('clang++' if tool == 'clangxx' else tool)
        if not executable or not Path(executable).is_file():
            parser.error(f'Missing {tool}; use the explicit --{tool} argument')
        resolved[tool] = executable
    command = [resolved['cmake'], '-S', str(source), '-B', str(build), '-G', 'Ninja',
               '-DCMAKE_MAKE_PROGRAM=' + resolved['ninja'],
               '-DCMAKE_C_COMPILER=' + resolved['clang'], '-DCMAKE_CXX_COMPILER=' + resolved['clangxx'],
               '-DCMAKE_C_COMPILER_TARGET=i686-pc-windows-msvc',
               '-DCMAKE_CXX_COMPILER_TARGET=i686-pc-windows-msvc',
               '-DCMAKE_SYSTEM_NAME=Windows', '-DCMAKE_SYSTEM_PROCESSOR=x86',
               '-DCMAKE_BUILD_TYPE=Release', '-DBUILD_SHARED_LIBS=OFF',
               '-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL',
               '-DPython3_EXECUTABLE=' + sys.executable, '-DPython_EXECUTABLE=' + sys.executable,
               '-DCMAKE_CROSSCOMPILING_EMULATOR=' + sys.executable + ';' + str(target_runner),
               '-DRust_CARGO=' + resolved['cargo'], '-DRust_COMPILER=' + resolved['rustc'],
               '-DRust_CARGO_TARGET=i686-pc-windows-msvc', '-DRust_RESOLVE_RUSTUP_TOOLCHAINS=OFF',
               '-DAURORA_ENABLE_GX=ON', '-DAURORA_ENABLE_DVD=ON', '-DAURORA_ENABLE_CARD=OFF',
               '-DAURORA_DAWN_PROVIDER=vendor', '-DAURORA_DAWN_LINKAGE=shared',
               '-DAURORA_SDL3_PROVIDER=package', '-DAURORA_SDL3_LINKAGE=shared',
               '-DAURORA_NOD_PROVIDER=vendor', '-DAURORA_NOD_LINKAGE=shared']
    dependency_sources = {}
    if args.dependency_source_map:
        mapping = json.loads(args.dependency_source_map.read_text(encoding='utf-8'))
        if not isinstance(mapping, dict):
            parser.error('Dependency map must be a JSON object')
        for name, value in mapping.items():
            if not name or any(c not in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-' for c in name):
                parser.error(f'Invalid FetchContent name: {name}')
            path = Path(value).resolve()
            if not path.is_dir():
                parser.error(f'Missing dependency source: {name}')
            dependency_sources[name] = str(path)
            command.append(f'-DFETCHCONTENT_SOURCE_DIR_{name}={path}')
        command.append('-DFETCHCONTENT_FULLY_DISCONNECTED=ON')
        command.append('-DDAWN_FETCH_DEPENDENCIES=OFF')
    env = os.environ.copy()
    env.pop('AURORA_SRC', None)
    env.pop('CARGO_TARGET_DIR', None)
    env['CARGO_BUILD_JOBS'] = str(args.jobs)
    build.mkdir(parents=True, exist_ok=True)
    record = dict(source=str(source), build=str(build), command=command,
                  dependency_sources=dependency_sources, configure_passed=False, build_passed=False,
                  scope='Renderer runtime only; no game executable, asset bundle or gameplay acceptance')
    evidence = build / 'runtime-rebuild-record.json'
    evidence.write_text(json.dumps(record, indent=2), encoding='utf-8')
    print('Configuring x86 Release native runtime; memory-card backend disabled.', flush=True)
    result = subprocess.run(command, env=env)
    record['configure_exit_code'] = result.returncode
    record['configure_passed'] = result.returncode == 0
    evidence.write_text(json.dumps(record, indent=2), encoding='utf-8')
    if result.returncode:
        raise SystemExit(result.returncode)
    if not args.configure_only:
        result = subprocess.run([resolved['cmake'], '--build', str(build), '--parallel', str(args.jobs)], env=env)
        record['build_exit_code'] = result.returncode
        record['build_passed'] = result.returncode == 0
        evidence.write_text(json.dumps(record, indent=2), encoding='utf-8')
        if result.returncode:
            raise SystemExit(result.returncode)


if __name__ == '__main__':
    main()
