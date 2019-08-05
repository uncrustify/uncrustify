#
# Generate uncrustify_version.h from uncrustify_version.h.in
#
# This script is meant to be executed with `cmake -P` from a custom target,
# and expects the variables `PYTHON_EXECUTABLE`, `SOURCE_DIR`, `INPUT`,
# `OUTPUT`, `DEBUG` and `CURRENT_VERSION` to be set.
#


execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${SOURCE_DIR}/scripts/make_version.py ${DEBUG}
  WORKING_DIRECTORY ${SOURCE_DIR}
  RESULT_VARIABLE make_version_error
  OUTPUT_VARIABLE make_version_output
)

if (make_version_error)
  message(STATUS "scripts/make_version.py exited with code ${make_version_error}: ${make_version_output}\n"
                  "As a fallback, version '${CURRENT_VERSION}' will be used.")
else()
  string(STRIP ${make_version_output} CURRENT_VERSION)
  message(STATUS "Version: '${CURRENT_VERSION}'")
endif()

configure_file("${INPUT}" "${OUTPUT}" @ONLY)
