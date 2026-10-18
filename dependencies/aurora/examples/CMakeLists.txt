if (AURORA_ENABLE_GX)
  add_executable(simple simple.c)
  # Core's native controls UI uses PAD bindings even in the standalone sample.
  target_link_libraries(simple PRIVATE aurora::core aurora::gx aurora::main aurora::vi aurora::pad)
  include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/AuroraCopyRuntimeDLLs.cmake)
  aurora_copy_runtime_dlls(simple)
endif ()
