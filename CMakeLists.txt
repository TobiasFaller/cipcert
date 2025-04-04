cmake_minimum_required(VERSION 3.10)
project(cipcert VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)
file(GLOB headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB sources CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM sources
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cipsim.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/cipcert.cpp)
include(util.cmake)

option(TOOLS "Build additional tools (kissat, scripts)" ON)
option(STATIC "Build static binary" OFF)
option(ASAN "Build with address sanitizer" OFF)

add_executable(cipcert ${sources} src/cipcert.cpp)
target_compile_options(cipcert PRIVATE -DGITID="${GIT_ID}")
target_compile_options(cipcert PRIVATE -DVERSION="${PROJECT_VERSION}")

add_executable(cipsim ${sources} src/cipsim.cpp)
target_compile_options(cipsim PRIVATE -DGITID="${GIT_ID}")
target_compile_options(cipsim PRIVATE -DVERSION="${PROJECT_VERSION}")

if(TOOLS)
  include(dep/quabs/quabs_bin.cmake)
  include(dep/cipfuzz/cipfuzz_bin.cmake)
  add_scripts(
    check_unsafe
    check_safe
    check
    certified
    random
    fuzz
    fuzzer)
endif()

if(STATIC)
  set(BUILD_SHARED_LIBS OFF)
  set_target_properties(cipcert PROPERTIES LINK_FLAGS "-static")
  set_target_properties(cipsim PROPERTIES LINK_FLAGS "-static")
endif()

if(ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

add_custom_target(cipcert_run DEPENDS cipcert
  COMMAND ./bin/cipcert ../examples/model.cip ../examples/witness.cip)

add_custom_target(cipcert_smoketest DEPENDS cipcert
  COMMAND ./bin/check ../examples/model.cip ../examples/witness.cip)
