cmake_minimum_required(VERSION 3.5)
project(dimcert VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 23)
file(GLOB headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB sources CONFIGURE_DEPENDS src/*.cpp)
include(util.cmake)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(TOOLS "Build additional tools (kissat, scripts)" OFF)
option(STATIC "Build static binary" OFF)

add_executable(dimcert ${sources})
target_compile_options(dimcert PRIVATE -DGITID="${GIT_ID}")
target_compile_options(dimcert PRIVATE -DVERSION="${PROJECT_VERSION}")

if(TOOLS)
  include(dep/kissat/kissat_bin.cmake)
  add_scripts(check_unsafe check_safe check certified)
endif()

if(STATIC)
  set(BUILD_SHARED_LIBS OFF)
  set_target_properties(dimcert PROPERTIES LINK_FLAGS "-static")
endif()

add_custom_target(run DEPENDS dimcert COMMAND ./bin/dimcert ../examples/model.dimspec
                                              ../examples/witness.dimspec)
