# Game sources and the headers/definitions they are built against
include_guard(GLOBAL)

get_filename_component(_melee_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# From the dev shell's environment when it has it, else as cached: ninja
# re-runs CMake from wherever it is started (e.g. objdiff)
if(DEFINED ENV{AURORA_SRC})
    set(AURORA_SRC "$ENV{AURORA_SRC}" CACHE PATH "Aurora's source" FORCE)
endif()
if(NOT AURORA_SRC)
    message(FATAL_ERROR "AURORA_SRC is not set: configure from the dev shell")
endif()

add_library(melee_game_headers INTERFACE)
target_include_directories(melee_game_headers INTERFACE
    ${AURORA_SRC}/include
    ${_melee_root}/src
    ${_melee_root}/libs/doldecomp/include
)
target_compile_definitions(melee_game_headers INTERFACE
    TARGET_PC
    bool=int
)

option(MELEE_UNLOCK_ALL "Unlock all playable characters and stages in the PC port" ON)
option(MELEE_NO_SAVES "Use session-only data without memory cards in the PC port" ON)
if(MELEE_NO_SAVES)
    target_compile_definitions(melee_game_headers INTERFACE MELEE_NO_SAVES=1)
endif()
if(MELEE_UNLOCK_ALL)
    target_compile_definitions(melee_game_headers INTERFACE MELEE_UNLOCK_ALL=1)
endif()

file(GLOB_RECURSE SOURCES "${_melee_root}/src/melee/**.c" LIST_DIRECTORIES FALSE)
file(GLOB_RECURSE HSD_SOURCES "${_melee_root}/src/sysdolphin/**.c" LIST_DIRECTORIES FALSE)
set(SOURCES ${SOURCES} ${HSD_SOURCES})

set(EXCLUSIONS
    sysdolphin/baselib/hsd_392C.c # Console EXI USB server; native unavailable backend
    sysdolphin/baselib/hsd_3933.c # Console MCC/FIO server; native unavailable backend
    melee/db/dberror.c # OSContext register usage
    melee/gm/gmmain.c # CARDInit
    melee/lb/lbcardnew.c # CARD ASync
    sysdolphin/baselib/debug.c # PPC register usage
    sysdolphin/baselib/debugconsole_main.c # PPC register usage
    sysdolphin/baselib/hsd_3915.c # requires debug font from dol
    sysdolphin/baselib/pobj.c # nontrivial GXSetArray usage
    sysdolphin/baselib/sislib_font.c # requires font from dol
)

set(MELEE_GENERATED_INCLUDE_DIR "" CACHE PATH "Local includes extracted from a verified user disc")
list(REMOVE_ITEM EXCLUSIONS melee/lb/lbcardnew.c)
list(REMOVE_ITEM EXCLUSIONS melee/gm/gmmain.c)
list(REMOVE_ITEM EXCLUSIONS sysdolphin/baselib/pobj.c)
if(MELEE_GENERATED_INCLUDE_DIR)
    foreach(_font debug_font sislib_font)
        if(NOT EXISTS "${MELEE_GENERATED_INCLUDE_DIR}/sysdolphin/baselib/${_font}.inc")
            message(FATAL_ERROR "Missing ${_font}.inc; run tools/extract_pc_fonts.py")
        endif()
    endforeach()
    target_include_directories(melee_game_headers INTERFACE "${MELEE_GENERATED_INCLUDE_DIR}")
    list(REMOVE_ITEM EXCLUSIONS sysdolphin/baselib/hsd_3915.c sysdolphin/baselib/sislib_font.c)
endif()

foreach(_exclusion IN LISTS EXCLUSIONS)
    list(REMOVE_ITEM SOURCES "${_melee_root}/src/${_exclusion}")
endforeach()

add_library(melee STATIC ${SOURCES})
target_sources(melee PRIVATE "${_melee_root}/src/pc/movie_stream.c")
target_sources(melee PRIVATE "${_melee_root}/src/pc/movie_decode.c")
target_sources(melee PRIVATE "${_melee_root}/src/pc/thp_native.c")
target_sources(melee PRIVATE "${_melee_root}/src/pc/host_startup.c")
target_sources(melee PRIVATE "${_melee_root}/src/pc/renderer_restart.c")
# Compile the original movie player against its upstream THP declarations.
# Keep this fallback scoped to these files; the host SDK remains Aurora.
# A real native decoder must resolve the THP entrypoints before full-game link.
set_source_files_properties(
    "${_melee_root}/src/melee/lb/lb_01F8.c"
    "${_melee_root}/src/melee/lb/lbmthp.c"
    "${_melee_root}/src/pc/thp_native.c"
    PROPERTIES INCLUDE_DIRECTORIES "${_melee_root}/libs/dolphin/include")
# lb_0195 uses the native owner-thread alarm/interrupt gate. Keep its original
# startup and input queue entrypoints in the game library; reset dependencies
# must be supplied by the host rather than omitting the scheduler from the port.
if(NOT MELEE_DWARF)
    target_sources(melee PRIVATE "${_melee_root}/src/pc/video_config.c" "${_melee_root}/src/pc/frame_clock.c" "${_melee_root}/src/MSL/float.c" "${_melee_root}/src/pc/render_state.c" "${_melee_root}/src/pc/heap_state.c" "${_melee_root}/src/pc/debug_native.c" "${_melee_root}/src/pc/db_debug_native.c" "${_melee_root}/src/pc/development_transport.c" "${_melee_root}/src/pc/thread_diagnostics.c" "${_melee_root}/src/pc/os_report.c" "${_melee_root}/src/pc/asset_regions.c" "${_melee_root}/src/pc/archive_relocate.c")
endif()
# Owner and monotonic clock must be installed by the native host before SDK use.
# These callbacks model owner-thread dispatch, not GameCube hardware interrupts.
if(NOT MELEE_DWARF)
    target_sources(melee PRIVATE
        "${_melee_root}/src/pc/pad_sampling.c"
        "${_melee_root}/src/pc/pad_poll_backend.c"
        "${_melee_root}/src/pc/pad_poll_policy.c"
        "${_melee_root}/src/pc/callback_gate.c"
        "${_melee_root}/src/pc/reset_host.c"
        "${_melee_root}/src/pc/os_alarm.c"
        "${_melee_root}/src/pc/alarm_queue.c"
        "${_melee_root}/src/pc/alarm_deadline.c"
        "${_melee_root}/src/pc/alarm_clock.c"
        "${_melee_root}/src/pc/tick_clock.c"
    )
endif()
if(MELEE_NO_SAVES)
    target_sources(melee PRIVATE "${_melee_root}/src/pc/card_disabled.c")
endif()
target_link_libraries(melee PRIVATE melee_game_headers)










