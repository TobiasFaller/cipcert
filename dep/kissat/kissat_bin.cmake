include(ExternalProject)
ExternalProject_Add(
  kissat
  GIT_REPOSITORY https://github.com/biere/kissat.git
  GIT_TAG main
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./configure.sh
  BUILD_COMMAND make -j
  INSTALL_COMMAND cp build/kissat ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
