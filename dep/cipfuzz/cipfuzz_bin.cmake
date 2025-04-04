include(ExternalProject)
ExternalProject_Add(
  cipfuzz
  GIT_REPOSITORY https://github.com/TobiasFaller/cipfuzz.git
  GIT_TAG main
  BUILD_IN_SOURCE 1
  UPDATE_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make -j
  INSTALL_COMMAND cp cipfuzz ${CMAKE_CURRENT_BINARY_DIR}
)
