include(ExternalProject)
ExternalProject_Add(
  dimfuzz
  GIT_REPOSITORY https://github.com/Froleyks/dimfuzz.git
  GIT_TAG main
  BUILD_IN_SOURCE 1
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make -j
  INSTALL_COMMAND cp dimfuzz ${CMAKE_CURRENT_BINARY_DIR}
)
