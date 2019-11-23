#
# Generate uncrustify_version.h from uncrustify_version.h.in
#
# This script is meant to be executed with `cmake -P` from a custom target,
# and expects the variables `PYTHON_EXECUTABLE`, `SOURCE_DIR`, `INPUT`,
# `OUTPUT` and `UNCRUSTIFY_VERSION` to be set.
#


execute_process(
  COMMAND ${PYTHON_EXECUTABLE} ${SOURCE_DIR}/scripts/make_version.py
  WORKING_DIRECTORY ${SOURCE_DIR}
  RESULT_VARIABLE make_version_error
  OUTPUT_VARIABLE make_version_output
)

if (make_version_error)
  # It's normal for make_version.py to fail when building from a tarball, so we
  # want to avoid anything that looks too much like a scary error. Thus, report
  # the error in an innocuous-looking fashion.
  #
  # If make_version.py is failing unexpectedly and needs to be debugged,
  # uncomment the next few lines.
  # string(STRIP "${make_version_output}" make_version_output)
  # message(STATUS
  #   "scripts/make_version.py exited with code ${make_version_error}: "
  #   "${make_version_output}")

  message(STATUS
    "Unable to determine version from source tree; "
    "fallback version '${UNCRUSTIFY_VERSION}' will be used")
  message(STATUS
    "(This is normal if you are building from a zip / tarball)")
else()
  string(STRIP ${make_version_output} UNCRUSTIFY_VERSION)
  message(STATUS "Version: '${UNCRUSTIFY_VERSION}'")
endif()

configure_file("${INPUT}" "${OUTPUT}" @ONLY)
