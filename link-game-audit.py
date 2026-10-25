"""Link diagnostic only; never launches the resulting target."""
import argparse, json, pathlib, re, subprocess
root = pathlib.Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime-build', type=pathlib.Path, default=root/'build-aurora-x86')
parser.add_argument('--game-library', type=pathlib.Path, default=root/'build-melee-win32/melee.lib')
parser.add_argument('--output', type=pathlib.Path, default=root/'melee-link-audit.exe')
parser.add_argument('--host-object', type=pathlib.Path, help='Developer fixture host object; does not replace the production library')
parser.add_argument('--legacy-2gb-address-space', action='store_true',
                    help='Diagnostic comparison only: disable large-address support for the x86 executable')
save_mode = parser.add_mutually_exclusive_group()
save_mode.add_argument('--no-saves', dest='no_saves', action='store_true',
                       help='Select the no-save link plan and omit Aurora card storage')
save_mode.add_argument('--with-saves', dest='no_saves', action='store_false',
                       help='Use the save-enabled link plan from the game build cache')
parser.set_defaults(no_saves=None)
parser.add_argument('--plan-only', action='store_true', help='Print the resolved library plan without linking or launching')
args = parser.parse_args()
runtime = args.runtime_build.resolve()
game_library = args.game_library.resolve()
output = args.output.resolve()
if not (runtime/'build.ninja').is_file() or not game_library.is_file():
    parser.error('Existing Aurora Ninja build and compiled game library are required; see NATIVE_REBUILD.txt.')
def cache_bool(path, name, default):
    if not path.is_file(): return default
    match = re.search(r'^'+re.escape(name)+r':BOOL=(ON|OFF)$', path.read_text(), re.M)
    return match.group(1)=='ON' if match else default
def cache_string(path, name, default):
    if not path.is_file(): return default
    match = re.search(r'^'+re.escape(name)+r':(?:STRING|FILEPATH|PATH|UNINITIALIZED)=(.*)$', path.read_text(), re.M)
    return match.group(1).strip() if match else default
game_cache = game_library.parent/'CMakeCache.txt'
no_saves = args.no_saves if args.no_saves is not None else cache_bool(game_cache,'MELEE_NO_SAVES',False)
runtime_cache = runtime/'CMakeCache.txt'
card_enabled = cache_bool(runtime_cache,'AURORA_ENABLE_CARD',True)
dvd_enabled = cache_bool(runtime_cache,'AURORA_ENABLE_DVD',True)
build_type = cache_string(runtime_cache,'CMAKE_BUILD_TYPE','Debug') or 'Debug'
is_debug = build_type.lower() == 'debug'
crt = 'msvcrtd' if is_debug else 'msvcrt'
compiler = cache_string(runtime_cache,'CMAKE_CXX_COMPILER','clang++') or 'clang++'
game_build_type = cache_string(game_cache,'CMAKE_BUILD_TYPE','')
if game_build_type and (game_build_type.lower() == 'debug') != is_debug:
    parser.error(f'Game library uses {game_build_type}, but Aurora uses {build_type}. '
                 'Rebuild both with the same build type; Debug and Release C++ runtimes cannot be mixed.')
output.parent.mkdir(parents=True, exist_ok=True)
ninja = (runtime/'build.ninja').read_text()
block = ninja.split('build examples/simple.exe:',1)[1].split('\nbuild ',1)[0]
libs = re.search(r'^  LINK_LIBRARIES = (.*)$',block,re.M).group(1).split()
libs = [lib for lib in libs if lib != 'aurora_main.lib']
if no_saves:
    libs = [lib for lib in libs if pathlib.Path(lib).name.lower() != 'aurora_card.lib']
for name in ['aurora_ax.lib','aurora_vi.lib','aurora_os.lib','aurora_pad.lib','aurora_mtx.lib','aurora_si.lib']:
    if name not in libs: libs.insert(0,name)
if card_enabled and not no_saves and 'aurora_card.lib' not in libs:
    libs.insert(0,'aurora_card.lib')
if dvd_enabled:
    if 'aurora_dvd.lib' not in libs: libs.insert(0,'aurora_dvd.lib')
    # Include transitive native DVD dependencies, not just its archive.
    marker = 'build melee_disc_smoke.exe:'
    if marker in ninja:
        dvd_block = ninja.split(marker,1)[1].split('\nbuild ',1)[0]
        match = re.search(r'^  LINK_LIBRARIES = (.*)$',dvd_block,re.M)
        if match:
            for lib in match.group(1).split():
                if lib not in libs: libs.append(lib)
    # When the runtime build has no game-specific smoke target, recover the
    # explicit nod target from the pinned Aurora build. The simple sample does
    # not link aurora_dvd, so its dependency list alone is incomplete.
    nod_linkage = cache_string(runtime_cache,'AURORA_NOD_LINKAGE','shared').lower()
    nod_dir = runtime/'_deps'/'aurora_nod-build'
    nod_library = nod_dir/('nod.dll.lib' if nod_linkage == 'shared' else 'nod.lib')
    nod_relative = nod_library.relative_to(runtime).as_posix()
    if nod_library.is_file() and nod_relative not in libs:
        libs.insert(0,nod_relative)
plan = {'no_saves':no_saves,'large_address_aware':not args.legacy_2gb_address_space,'aurora_card_enabled':card_enabled,
        'aurora_card_linked':any(pathlib.Path(lib).name.lower()=='aurora_card.lib' for lib in libs),
        'aurora_dvd_enabled':dvd_enabled,'nod_linkage':cache_string(runtime_cache,'AURORA_NOD_LINKAGE','shared'),
        'build_type':build_type,'game_build_type':game_build_type,'compiler':compiler,
        'crt':crt,'libraries':libs}
if args.plan_only:
    print(json.dumps(plan,indent=2))
    if plan['no_saves'] and plan['aurora_card_linked']:
        raise SystemExit('No-save link plan must not include aurora_card.lib')
    raise SystemExit(0)
command = [compiler,'--target=i686-pc-windows-msvc','-nostartfiles','-nostdlib',
           '-D_DEBUG' if is_debug else '-DNDEBUG','-D_DLL','-D_MT','-Xclang',f'--dependent-lib={crt}',
           '-fuse-ld=lld-link','-Xlinker','/subsystem:console','-Xlinker','/errorlimit:0',
           '-Xlinker','/machine:x86','-Xlinker',f'/defaultlib:{crt}','-Xlinker','/include:_main',
           '-Xlinker','/largeaddressaware:no' if args.legacy_2gb_address_space else '/largeaddressaware',
           str(game_library),*libs,'-o',str(output)]
command.extend(['-Xlinker','/map:'+str(output.with_suffix('.map'))])
if args.host_object:
    command.insert(command.index(str(game_library)),str(args.host_object.resolve()))
proc = subprocess.run(command,cwd=runtime,capture_output=True,text=True)
output.with_suffix('.log').write_text(proc.stdout+proc.stderr)
undefined = sorted(set(re.findall(r'undefined symbol: (.+)',proc.stderr)))
report = {'exit_code':proc.returncode,'undefined_count':len(undefined),'undefined_symbols':undefined,
          'link_plan':plan,'note':'Native original-game host link. The executable has a normal startup path and is run separately; a successful link is not evidence of a playable release.'}
report.update(runtime_build=str(runtime), game_library=str(game_library), output=str(output), launched=False)
output.with_suffix('.json').write_text(json.dumps(report,indent=2))
print(json.dumps({'exit_code':proc.returncode,'undefined_count':len(undefined)}))
raise SystemExit(proc.returncode)



