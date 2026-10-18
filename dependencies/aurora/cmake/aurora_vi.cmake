add_library(aurora_vi STATIC
        lib/dolphin/vi/vi.cpp
)
add_library(aurora::vi ALIAS aurora_vi)
set_target_properties(aurora_vi PROPERTIES FOLDER "aurora")

target_link_libraries(aurora_vi PUBLIC aurora::core)
if (AURORA_ENABLE_GX)
    target_link_libraries(aurora_vi PRIVATE aurora::gx absl::flat_hash_map absl::btree)
    target_compile_definitions(aurora_vi PRIVATE WEBGPU_DAWN)
endif ()
