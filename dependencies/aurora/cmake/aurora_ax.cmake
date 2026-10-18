add_library(aurora_ax STATIC lib/dolphin/AX.cpp lib/dolphin/AXFX.cpp lib/dolphin/AXFXReverbStd.cpp lib/dolphin/AXFXReverbHi.cpp lib/dolphin/AXFXChorus.cpp)
add_library(aurora::ax ALIAS aurora_ax)
set_target_properties(aurora_ax PROPERTIES FOLDER "aurora")
target_link_libraries(aurora_ax PUBLIC aurora::core aurora::os)
# The owner services a real-time 5ms audio block. Keep Debug assertions/CRT,
# but optimize sample mixing and effect kernels so 64 voices fit that budget.
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(aurora_ax PRIVATE $<$<CONFIG:Debug>:-O2>)
elseif(MSVC)
  target_compile_options(aurora_ax PRIVATE $<$<CONFIG:Debug>:/O2>)
endif()
