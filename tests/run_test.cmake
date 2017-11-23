#
# This is a wrapper cmake script to run one uncrustify test because cmake add_test
# can't launch multiple processes and so it doesn't allow a complex test scenario.
#

cmake_minimum_required(VERSION 2.8)


if(NOT TEST_PROGRAM)
  message( FATAL_ERROR "Variable TEST_PROGRAM not defined" )
endif()
if(NOT TEST_LANG)
  message( FATAL_ERROR "Variable TEST_LANG not defined" )
endif()
if(NOT TEST_CONFIG)
  message( FATAL_ERROR "Variable TEST_CONFIG not defined" )
endif()
if(NOT TEST_RERUN_CONFIG)
  message( FATAL_ERROR "Variable TEST_RERUN_CONFIG not defined" )
endif()
if(NOT TEST_INPUT)
  message( FATAL_ERROR "Variable TEST_INPUT not defined" )
endif()
if(NOT TEST_OUTPUT)
  message( FATAL_ERROR "Variable TEST_OUTPUT not defined" )
endif()
if(NOT TEST_RESULT)
  message( FATAL_ERROR "Variable TEST_RESULT not defined" )
endif()
if(NOT TEST_RESULT_2)
  message( FATAL_ERROR "Variable TEST_RESULT_2 not defined" )
endif()
if(NOT TEST_DIR)
  message( FATAL_ERROR "Variable TEST_DIR not defined" )
endif()


execute_process(
  COMMAND ${TEST_PROGRAM} -l ${TEST_LANG} -c ${TEST_CONFIG} -f ${TEST_INPUT} -o ${TEST_RESULT}
  WORKING_DIRECTORY ${TEST_DIR}
  RESULT_VARIABLE uncrustify_error_code
  OUTPUT_QUIET
  ERROR_VARIABLE uncrustify_error
)

if(uncrustify_error_code)
  message( WARNING "Uncrustify error(Pass 1): ${uncrustify_error}" )
endif(uncrustify_error_code)

# Unfortunately I had to pull cmake out of the previous execute_process due to instability issues.
execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_DIR}/${TEST_RESULT} ${TEST_DIR}/${TEST_OUTPUT}
  RESULT_VARIABLE files_are_different
)

if(files_are_different)
  message( WARNING "MISSMATCH (Pass 1): ${TEST_RESULT} does not match ${TEST_OUTPUT}" )
endif()
execute_process(
  COMMAND ${CMAKE_COMMAND} -E echo "MISSMATCH for the test input file: ${TEST_INPUT}"
)
if(files_are_different)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy ${TEST_DIR}/${TEST_RESULT} /dev/stderr
  )
endif()

# Re-run with the output file as the input to check stability.
execute_process(
  COMMAND ${TEST_PROGRAM} -l ${TEST_LANG} -c ${TEST_RERUN_CONFIG} -f ${TEST_OUTPUT} -o ${TEST_RESULT_2}
  WORKING_DIRECTORY ${TEST_DIR}
  RESULT_VARIABLE uncrustify_error_code
  OUTPUT_QUIET
  ERROR_VARIABLE uncrustify_error
)

if(uncrustify_error_code)
  message( WARNING "Uncrustify error(Pass 2): ${uncrustify_error}")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_DIR}/${TEST_RESULT_2} ${TEST_DIR}/${TEST_OUTPUT}
  RESULT_VARIABLE files_are_different
)

if(files_are_different)
  message(WARNING "UNSTABLE(Pass 2): ${TEST_RESULT_2} does not match ${TEST_OUTPUT}")
endif()
