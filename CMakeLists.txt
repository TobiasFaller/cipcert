cmake_minimum_required(VERSION 3.10)
project(dimcert VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)
file(GLOB headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB sources CONFIGURE_DEPENDS src/*.cpp)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/dimsim.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/src/dimcert.cpp)
include(util.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(TOOLS "Build additional tools (kissat, scripts)" ON)
option(STATIC "Build static binary" OFF)
option(ASAN "Build with address sanitizer" OFF)

add_executable(dimcert ${sources} src/dimcert.cpp)
target_compile_options(dimcert PRIVATE -DGITID="${GIT_ID}")
target_compile_options(dimcert PRIVATE -DVERSION="${PROJECT_VERSION}")

add_executable(dimsim ${sources} src/dimsim.cpp)
target_compile_options(dimsim PRIVATE -DGITID="${GIT_ID}")
target_compile_options(dimsim PRIVATE -DVERSION="${PROJECT_VERSION}")

if(TOOLS)
  # include(dep/kissat/kissat_bin.cmake)
  include(dep/quabs/quabs_bin.cmake)
  include(dep/dimfuzz/dimfuzz_bin.cmake)
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
  set_target_properties(dimcert PROPERTIES LINK_FLAGS "-static")
  set_target_properties(dimsim PROPERTIES LINK_FLAGS "-static")
endif()

if(ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

add_custom_target(dimcert_run DEPENDS dimcert COMMAND ./bin/dimcert ../examples/model.dimspec
                                                      ../examples/witness.dimspec)

add_custom_target(smoketest DEPENDS dimcert COMMAND ./bin/check ../examples/model.dimspec
                                                    ../examples/witness.dimspec)
