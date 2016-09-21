file(APPEND "${UNITTEST_OUTPUT}" "EXIT STATUS: ${UNITTEST_STATUS}\n")

if (${UNITTEST_STATUS})
    get_filename_component(name "${UNITTEST_OUTPUT}" NAME_WE)
    message("\n====== BEGIN OUTPUT ====== ${name}")
    execute_process(${CMD_CAT} "${UNITTEST_OUTPUT}")
    message("====== END OUTPUT ====== ${name}\n")
endif()
