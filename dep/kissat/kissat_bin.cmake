include(ExternalProject)
ExternalProject_Add(
  kissat
  GIT_REPOSITORY https://github.com/arminbiere/kissat.git
  GIT_TAG master
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./configure
  BUILD_COMMAND make -j
  INSTALL_COMMAND cp build/kissat ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)
