include(FetchContent)
include(GoogleTest)
include(${CMAKE_CURRENT_LIST_DIR}/../cmake/AuroraCopyRuntimeDLLs.cmake)

set(CMAKE_GTEST_DISCOVER_TESTS_DISCOVERY_MODE PRE_TEST)

if (NOT TARGET gtest)
  FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    EXCLUDE_FROM_ALL
  )
  FetchContent_MakeAvailable(googletest)
endif ()

add_executable(io_tests
  io_test.cpp
  ../lib/card/FileIO.cpp
  ../lib/io.cpp
)
target_include_directories(io_tests PRIVATE ../lib)
target_link_libraries(io_tests PRIVATE gtest gtest_main ${AURORA_SDL3_TARGET})
aurora_copy_runtime_dlls(io_tests)
gtest_discover_tests(io_tests)

# Header-only CPU checks for the advanced 50%/25% in-game texture settings.
add_executable(texture_downsample_tests texture_downsample_test.cpp)
target_include_directories(texture_downsample_tests PRIVATE ../lib)
add_test(NAME texture_downsample_tests COMMAND texture_downsample_tests)

if (AURORA_ENABLE_GX)
  set(_aurora_gtest_library gtest)
  set(_aurora_gtest_main_library gtest_main)
  set(_aurora_build_pinned_gtest OFF)
  if (NOT TARGET gtest OR NOT TARGET gtest_main)
    set(_aurora_build_pinned_gtest ON)
  else()
    get_target_property(_aurora_gtest_includes gtest INTERFACE_INCLUDE_DIRECTORIES)
    if (NOT _aurora_gtest_includes)
      set(_aurora_build_pinned_gtest ON)
    endif()
  endif()

  # Dawn can expose placeholder gtest target names without usable headers or
  # library locations. Fall back to a private target built from Aurora's
  # pinned GoogleTest archive only when the existing target lacks headers.
  if (_aurora_build_pinned_gtest)
    FetchContent_Declare(aurora_pinned_googletest
      URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
      URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
      DOWNLOAD_EXTRACT_TIMESTAMP FALSE
    )
    FetchContent_GetProperties(aurora_pinned_googletest)
    if (NOT aurora_pinned_googletest_POPULATED)
      FetchContent_Populate(aurora_pinned_googletest)
    endif()
    add_library(aurora_pinned_gtest STATIC
      ${aurora_pinned_googletest_SOURCE_DIR}/googletest/src/gtest-all.cc
    )
    target_include_directories(aurora_pinned_gtest PUBLIC
      ${aurora_pinned_googletest_SOURCE_DIR}/googletest/include
    )
    target_include_directories(aurora_pinned_gtest PRIVATE
      ${aurora_pinned_googletest_SOURCE_DIR}/googletest
    )
    target_compile_features(aurora_pinned_gtest PUBLIC cxx_std_14)
    add_library(aurora_pinned_gtest_main STATIC
      ${aurora_pinned_googletest_SOURCE_DIR}/googletest/src/gtest_main.cc
    )
    target_link_libraries(aurora_pinned_gtest_main PUBLIC aurora_pinned_gtest)
    set(_aurora_gtest_library aurora_pinned_gtest)
    set(_aurora_gtest_main_library aurora_pinned_gtest_main)
  endif()

  # The display-copy CPU payload is reused per enrolled XFB slot. Keep a
  # focused lifecycle test independent from the GPU-backed GX test executable.
  add_executable(xfb_buffer_reuse_tests
    xfb_buffer_reuse_test.cpp
  )
  target_include_directories(xfb_buffer_reuse_tests PRIVATE ../lib)
  target_compile_definitions(xfb_buffer_reuse_tests PRIVATE AURORA TARGET_PC)
  target_link_libraries(xfb_buffer_reuse_tests PRIVATE ${AURORA_SDL3_TARGET})
  aurora_copy_runtime_dlls(xfb_buffer_reuse_tests)
  add_test(NAME xfb_buffer_reuse_tests COMMAND xfb_buffer_reuse_tests)

  # GX FIFO round-trip tests
  # Compile GX sources directly to avoid pulling in the full renderer/WebGPU runtime
  add_executable(gx_fifo_tests
    gx_fifo_test.cpp
    gx_test_stubs.cpp
    # GX API implementations (encoders)
    ../lib/dolphin/gx/GXBump.cpp
    ../lib/dolphin/gx/GXCull.cpp
    ../lib/dolphin/gx/GXCpu2Efb.cpp
    ../lib/dolphin/gx/GXDispList.cpp
    ../lib/dolphin/gx/GXDraw.cpp
    ../lib/dolphin/gx/GXExtra.cpp
    ../lib/dolphin/gx/GXFifo.cpp
    ../lib/dolphin/gx/GXFrameBuffer.cpp
    ../lib/dolphin/gx/GXGeometry.cpp
    ../lib/dolphin/gx/GXGet.cpp
    ../lib/dolphin/gx/GXLighting.cpp
    ../lib/dolphin/gx/GXManage.cpp
    ../lib/dolphin/gx/GXAurora.cpp
    ../lib/dolphin/gx/GXPerf.cpp
    ../lib/dolphin/gx/GXPixel.cpp
    ../lib/dolphin/gx/GXTev.cpp
    ../lib/dolphin/gx/GXTexture.cpp
    ../lib/dolphin/gx/GXTransform.cpp
    ../lib/dolphin/gx/GXVert.cpp
    # FIFO/command processor (encoder + decoder)
    ../lib/gx/fifo.cpp
    ../lib/gx/command_processor.cpp
    ../lib/gx/regs.cpp
    ../lib/thread.cpp
    # Display list reader/optimizer
    ../lib/gx/attr_fmt.cpp
    ../lib/gx/dl.cpp
    gx_dl_test.cpp
  )

  target_include_directories(gx_fifo_tests PRIVATE
    ../include
    ../lib
    ../lib/dolphin/gx
  )

  target_compile_definitions(gx_fifo_tests PRIVATE AURORA TARGET_PC)

  target_link_libraries(gx_fifo_tests PRIVATE
    gtest
    gtest_main
    fmt::fmt
    xxhash
    absl::flat_hash_map
    absl::btree
    dawn::dawncpp_headers
    TracyClient
    ${AURORA_SDL3_TARGET}
  )

  gtest_discover_tests(gx_fifo_tests)

  add_executable(gx_texture_cache_tests
    gx_texture_cache_test.cpp
    gx_texture_cache_test_stubs.cpp
    ../lib/gx/texture.cpp
  )
  target_include_directories(gx_texture_cache_tests PRIVATE
    ../include
    ../lib
  )
  target_compile_definitions(gx_texture_cache_tests PRIVATE AURORA TARGET_PC)
  target_link_libraries(gx_texture_cache_tests PRIVATE
    ${_aurora_gtest_main_library}
    fmt::fmt
    xxhash
    absl::flat_hash_map
    dawn::webgpu_dawn
    TracyClient
    ${AURORA_SDL3_TARGET}
  )
  aurora_copy_runtime_dlls(gx_texture_cache_tests)
  gtest_discover_tests(gx_texture_cache_tests)

  add_executable(texture_replacement_streaming_tests
    texture_replacement_streaming_test.cpp
  )
  target_include_directories(texture_replacement_streaming_tests PRIVATE
    ../include
    ../lib
  )
  target_compile_definitions(texture_replacement_streaming_tests PRIVATE AURORA TARGET_PC)
  target_link_libraries(texture_replacement_streaming_tests PRIVATE
    aurora::gx
    aurora::vi
    aurora::pad
    gtest
    gtest_main
  )
  aurora_copy_runtime_dlls(texture_replacement_streaming_tests)
  gtest_discover_tests(texture_replacement_streaming_tests)

  add_executable(render_worker_tests
    render_worker_test.cpp
    thread_test.cpp
    ../lib/gfx/render_worker.cpp
    ../lib/thread.cpp
  )
  target_include_directories(render_worker_tests PRIVATE
    ../include
    ../lib
  )
  target_compile_definitions(render_worker_tests PRIVATE AURORA TARGET_PC)
  target_link_libraries(render_worker_tests PRIVATE
    gtest
    gtest_main
    TracyClient
    ${AURORA_SDL3_TARGET}
  )
  gtest_discover_tests(render_worker_tests)
endif () # AURORA_ENABLE_GX

# DVD API tests
if (TARGET aurora::dvd)
  add_executable(dvd_tests
    test_dvd.cpp
    dvd_test_stubs.cpp
    ../lib/logging.cpp
  )
  target_link_libraries(dvd_tests PRIVATE aurora::dvd gtest gtest_main)
  aurora_copy_runtime_dlls(dvd_tests)

  # Pass disc image path as a compile definition if provided
  if (DEFINED DVD_TEST_IMAGE AND EXISTS "${DVD_TEST_IMAGE}")
    target_compile_definitions(dvd_tests PRIVATE DVD_TEST_IMAGE="${DVD_TEST_IMAGE}")
  endif ()

  gtest_discover_tests(dvd_tests)
endif ()

add_executable(os_alloc_tests
  os_alloc_test.cpp
  os_test_globals.cpp
  ../lib/dolphin/os/OSAlloc.cpp
  ../lib/logging.cpp
)
target_include_directories(os_alloc_tests PRIVATE
  ../include
)
target_compile_definitions(os_alloc_tests PRIVATE AURORA TARGET_PC)
target_link_libraries(os_alloc_tests PRIVATE gtest gtest_main fmt::fmt)
gtest_discover_tests(os_alloc_tests)
