if (WIN32)
  set(CMD_CAT cmd /c type)
else()
  set(CMD_CAT cat)
endif(WIN32)

file(APPEND "${UNITTEST_OUTPUT}" "EXIT STATUS: ${UNITTEST_STATUS}\n")

if (${UNITTEST_STATUS})
    get_filename_component(name "${UNITTEST_OUTPUT}" NAME_WE)
    message("\n====== BEGIN OUTPUT ====== ${name}")
    execute_process(COMMAND ${CMD_CAT} "${UNITTEST_OUTPUT}")
    message("====== END OUTPUT ====== ${name}\n")
endif()
