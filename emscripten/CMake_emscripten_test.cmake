if(NOT unc_targetfile)
    MESSAGE(FATAL_ERROR "unc_targetfile param not defined")
endif()
if(NOT unc_projdir )
    MESSAGE(FATAL_ERROR "unc_projdir param not defined")
endif()

message(STATUS "unc_targetfile: ${unc_targetfile}")
message(STATUS "test_dir:       ${unc_projdir}/emscripten/test/")
message(STATUS "-----------------------------------------------------------------------------")


find_package(PythonInterp REQUIRED)
execute_process(
  COMMAND ${PYTHON_EXECUTABLE} "${unc_projdir}/emscripten/test/run_tests.py" "${unc_targetfile}" "${unc_projdir}/emscripten/test/"
  WORKING_DIRECTORY ${unc_projdir}
  RESULT_VARIABLE make_version_error
  OUTPUT_VARIABLE make_version_output
)
MESSAGE(STATUS ${make_version_output})

if(NOT ${make_version_error} EQUAL 0)
  message(FATAL_ERROR "errno: ${make_version_error}")
endif()
