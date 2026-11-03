cmake_minimum_required(VERSION 3.25)
project(aurora_dawn_fetch_option_probe NONE)

if (NOT DEFINED EXPECT_FETCH)
  message(FATAL_ERROR "EXPECT_FETCH is required")
endif ()
if (SEED_LEGACY_OFF)
  set(DAWN_FETCH_DEPENDENCIES OFF CACHE INTERNAL "Legacy explicit OFF" FORCE)
endif ()
set(AURORA_DAWN_PROVIDER vendor)
set(AURORA_DAWN_LINKAGE shared)
set(AURORA_DAWN_REF 1155e0ed531126f33a1279afa029349651ca1c93)
# Intercept only dependency population. Execute the actual provider unchanged;
# this fixture never downloads, compiles or launches a renderer/game.
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/modules")
include("${CMAKE_CURRENT_LIST_DIR}/../../../cmake/AuroraDawnProvider.cmake")
if (NOT TARGET dawn::webgpu_dawn OR NOT TARGET dawn::dawncpp_headers)
  message(FATAL_ERROR "Provider did not retain its expected targets")
endif ()
