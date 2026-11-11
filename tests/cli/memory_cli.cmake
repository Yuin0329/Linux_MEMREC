if(NOT DEFINED LINXRAY OR NOT DEFINED FIXTURE_DIR)
    message(FATAL_ERROR "LINXRAY and FIXTURE_DIR are required")
endif()

function(check_cli name expected_exit stdout_pattern stderr_pattern)
    execute_process(
        COMMAND "${LINXRAY}" ${ARGN}
        RESULT_VARIABLE actual_exit
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        TIMEOUT 5
    )
    if(NOT "${actual_exit}" STREQUAL "${expected_exit}")
        message(FATAL_ERROR
            "${name}: expected exit ${expected_exit}, got ${actual_exit}\n${stdout}\n${stderr}")
    endif()
    if(NOT "${stdout_pattern}" STREQUAL "" AND NOT stdout MATCHES "${stdout_pattern}")
        message(FATAL_ERROR "${name}: unexpected stdout:\n${stdout}")
    endif()
    if(NOT "${stderr_pattern}" STREQUAL "" AND NOT stderr MATCHES "${stderr_pattern}")
        message(FATAL_ERROR "${name}: unexpected stderr:\n${stderr}")
    endif()
    if(expected_exit EQUAL 0 AND NOT stderr STREQUAL "")
        message(FATAL_ERROR "${name}: successful command wrote to stderr:\n${stderr}")
    endif()
    if(NOT expected_exit EQUAL 0 AND NOT stdout STREQUAL "")
        message(FATAL_ERROR "${name}: failed command wrote to stdout:\n${stdout}")
    endif()
endfunction()

check_cli(help 0 "Usage:" "" --help)
check_cli(no_command 0 "Usage:" "")
check_cli(memory_help 0 "--meminfo" "" memory --help)
check_cli(fixture 0
    "^Total: +8[.]00 GiB\nAvailable: +6[.]00 GiB\nUsed: +2[.]00 GiB\nUsage: +25[.]0%\n$"
    "" memory --meminfo "${FIXTURE_DIR}/meminfo.txt")
check_cli(unknown_command 2 "" "unknown command" cpu)
check_cli(unknown_option 2 "" "unknown option" memory --unknown)
check_cli(missing_path 2 "" "requires a file path" memory --meminfo)
check_cli(option_as_path 2 "" "requires a file path" memory --meminfo --help)
check_cli(duplicate_path 2 "" "only be provided once"
    memory --meminfo "${FIXTURE_DIR}/meminfo.txt" --meminfo "${FIXTURE_DIR}/meminfo.txt")
check_cli(missing_file 1 "" "cannot open meminfo file"
    memory --meminfo "${FIXTURE_DIR}/does-not-exist.meminfo")
check_cli(invalid_file 1 "" "MemAvailable cannot exceed MemTotal"
    memory --meminfo "${FIXTURE_DIR}/meminfo-invalid.txt")

message(STATUS "All CLI tests passed.")
