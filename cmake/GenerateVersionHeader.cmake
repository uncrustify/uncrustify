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

  # No git available in this branch -- VER_STRING can't carry a commit hash.
  set(GIT_SHORT_HASH "")
else()
  string(STRIP ${make_version_output} UNCRUSTIFY_VERSION)
  message(STATUS "Version: '${UNCRUSTIFY_VERSION}'")

  # git is known to work at this point (make_version.py just used it),
  # so grab the short hash of the current commit for VER_STRING below.
  execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${SOURCE_DIR}
    RESULT_VARIABLE git_hash_error
    OUTPUT_VARIABLE GIT_SHORT_HASH
  )
  if (git_hash_error)
    set(GIT_SHORT_HASH "")
  else()
    string(STRIP "${GIT_SHORT_HASH}" GIT_SHORT_HASH)
  endif()
endif()

# Derive VER_MAJOR/VER_MINOR/VER_PATCH/VER_STRING (for uncrustify.rc) from
# the same resolved UNCRUSTIFY_VERSION above, whichever branch produced it.
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ver_match "${UNCRUSTIFY_VERSION}")
if (NOT _ver_match)
  message(FATAL_ERROR
    "Could not parse major.minor.patch out of UNCRUSTIFY_VERSION "
    "'${UNCRUSTIFY_VERSION}'; cannot derive VER_MAJOR/VER_MINOR/VER_PATCH "
    "for uncrustify.rc")
endif()
set(VER_MAJOR ${CMAKE_MATCH_1})
set(VER_MINOR ${CMAKE_MATCH_2})
set(VER_PATCH ${CMAKE_MATCH_3})

if (GIT_SHORT_HASH)
  set(VER_STRING "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}+${GIT_SHORT_HASH}")

  # Also carry the hash on the CLI-facing UNCRUSTIFY_VERSION (used by
  # --version et al.) -- the fallback literal (e.g. "0.84.0_f") is left
  # untouched when no hash is available, so its "_f" marker still tells
  # fallback builds apart from a real, on-tag git build with no hash.
  set(UNCRUSTIFY_VERSION "${UNCRUSTIFY_VERSION}+${GIT_SHORT_HASH}")
else()
  set(VER_STRING "${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}")
endif()

configure_file("${INPUT}" "${OUTPUT}" @ONLY)
