set(OCCTDEBUG_DRAW_SMOKE_CONFIG "${CMAKE_BUILD_TYPE}")
if(CMAKE_CONFIGURATION_TYPES)
    set(OCCTDEBUG_DRAW_SMOKE_CONFIG "$<CONFIG>")
endif()

add_test(
    NAME draw_smoke
    COMMAND powershell
        -NoProfile
        -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/scripts/run_draw_smoke.ps1"
        -RepoRoot "${CMAKE_SOURCE_DIR}"
        -BuildDir "${CMAKE_BINARY_DIR}"
        -Config "${OCCTDEBUG_DRAW_SMOKE_CONFIG}"
)

set_tests_properties(draw_smoke PROPERTIES
    LABELS "draw;smoke;occt"
    FIXTURES_SETUP "draw_ready"
    PASS_REGULAR_EXPRESSION "DRAW_SMOKE_OK"
    FAIL_REGULAR_EXPRESSION "DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR"
)

add_test(
    NAME draw_checkshape_smoke
    COMMAND powershell
        -NoProfile
        -ExecutionPolicy Bypass
        -File "${CMAKE_SOURCE_DIR}/scripts/run_draw_smoke.ps1"
        -RepoRoot "${CMAKE_SOURCE_DIR}"
        -BuildDir "${CMAKE_BINARY_DIR}"
        -Config "${OCCTDEBUG_DRAW_SMOKE_CONFIG}"
        -TestName "draw_checkshape_smoke"
        -TclScript "${CMAKE_SOURCE_DIR}/tests/draw_checkshape_smoke.tcl"
        -SuccessToken "DRAW_CHECKSHAPE_OK"
)

set_tests_properties(draw_checkshape_smoke PROPERTIES
    LABELS "draw;smoke;occt;checkshape"
    FIXTURES_REQUIRED "draw_ready"
    PASS_REGULAR_EXPRESSION "DRAW_CHECKSHAPE_OK"
    FAIL_REGULAR_EXPRESSION "DRAW_SMOKE_FAILED|DRAW_SMOKE_ERROR|Faulty"
)
