if(NOT unc_targetfile)
    MESSAGE(FATAL_ERROR "unc_targetfile param not defined")
endif()

function(cat IN_FILE OUT_FILE)
  file(READ ${IN_FILE} CONTENTS)
  file(APPEND ${OUT_FILE} "${CONTENTS}")
endfunction()


SET(unc_tmpfile "${unc_targetfile}_.tmp")

file(WRITE "${unc_tmpfile}" "")

cat("${CMAKE_CURRENT_LIST_DIR}/prefix_file" "${unc_tmpfile}")
cat("${unc_targetfile}" "${unc_tmpfile}")
cat("${CMAKE_CURRENT_LIST_DIR}/postfix_file"  "${unc_tmpfile}")

file(RENAME "${unc_tmpfile}" "${unc_targetfile}")
