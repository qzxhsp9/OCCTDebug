# Qt6: include local defaults first, then CMAKE_PREFIX_PATH / cache (aligns with MovementPath usage).

set(_occtdebug_qt_defaults "${CMAKE_CURRENT_LIST_DIR}/QtWorkbenchDefaults.cmake")
if(EXISTS "${_occtdebug_qt_defaults}")
    include("${_occtdebug_qt_defaults}")
endif()

if(NOT OCCTDEBUG_QT_DEFAULT_KIT AND DEFINED MOVEMENTPATH_QT_WORKBENCH_DEFAULT_KIT)
    set(OCCTDEBUG_QT_DEFAULT_KIT "${MOVEMENTPATH_QT_WORKBENCH_DEFAULT_KIT}")
endif()
if(NOT OCCTDEBUG_QT_DEFAULT_KIT AND DEFINED ENV{OCCTDEBUG_QT_DEFAULT_KIT})
    set(OCCTDEBUG_QT_DEFAULT_KIT "$ENV{OCCTDEBUG_QT_DEFAULT_KIT}")
endif()

set(OCCTDEBUG_QT_ROOT
    ""
    CACHE PATH
    "Qt6 kit root (directory containing lib/cmake/Qt6). See QtWorkbenchDefaults.cmake.example.")

if(OCCTDEBUG_QT_ROOT)
    list(FIND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_ROOT}" _occtdebug_qt_prefix_idx)
    if(_occtdebug_qt_prefix_idx LESS 0)
        list(PREPEND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_ROOT}")
    endif()
endif()

if(OCCTDEBUG_QT_DEFAULT_KIT)
    list(FIND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_DEFAULT_KIT}" _occtdebug_qt_kit_idx)
    if(_occtdebug_qt_kit_idx LESS 0)
        list(PREPEND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_DEFAULT_KIT}")
    endif()
endif()

find_package(Qt6 6.5 QUIET COMPONENTS Core Gui Widgets)

if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "OCCTDebug: Qt6 (>= 6.5, Core/Gui/Widgets) was not found.\n"
        "Set OCCTDEBUG_QT_ROOT, add Qt to CMAKE_PREFIX_PATH, set Qt6_DIR, or set OCCTDEBUG_QT_DEFAULT_KIT in src/QtWorkbenchDefaults.cmake.\n"
        "OCCTDEBUG_QT_ROOT='${OCCTDEBUG_QT_ROOT}'\n"
        "OCCTDEBUG_QT_DEFAULT_KIT='${OCCTDEBUG_QT_DEFAULT_KIT}'\n"
        "CMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATH}'")
endif()

set(OCCTDEBUG_EFFECTIVE_QT_ROOT "${OCCTDEBUG_QT_ROOT}")
if(NOT OCCTDEBUG_EFFECTIVE_QT_ROOT AND Qt6_DIR)
    get_filename_component(OCCTDEBUG_EFFECTIVE_QT_ROOT "${Qt6_DIR}/../../.." ABSOLUTE)
endif()

set(OCCTDEBUG_SOURCES
    app/main.cpp
    core/app/AppContext.cpp
    core/app/AppContext.h
    core/case/CrashDumpArchive.cpp
    core/case/CrashDumpArchive.h
    core/case/CaseManifest.cpp
    core/case/CaseManifest.h
    core/case/CaseWorkspaceService.cpp
    core/case/CaseWorkspaceService.h
    core/config/ConfigService.cpp
    core/config/ConfigService.h
    core/evidence/EvidenceBundleWriter.cpp
    core/evidence/EvidenceBundleWriter.h
    core/geometry/OcctViewerWidget.cpp
    core/geometry/OcctViewerWidget.h
    core/geometry/TopologyCompareArtifact.cpp
    core/geometry/TopologyCompareArtifact.h
    core/geometry/TopologySignature.cpp
    core/geometry/TopologySignature.h
    core/knowledge/SimilarCaseSearch.cpp
    core/knowledge/SimilarCaseSearch.h
    core/Logger.cpp
    core/Logger.h
    core/patch/PatchReview.cpp
    core/patch/PatchReview.h
    core/report/MarkdownReportGenerator.cpp
    core/report/MarkdownReportGenerator.h
    core/repro/CppReproTemplateWriter.cpp
    core/repro/CppReproTemplateWriter.h
    core/repro/DrawLogParser.cpp
    core/repro/DrawLogParser.h
    core/repro/ReproStatusEvaluator.cpp
    core/repro/ReproStatusEvaluator.h
    core/runner/CommandRunner.cpp
    core/runner/CommandRunner.h
    core/runner/CommandTaskQueue.cpp
    core/runner/CommandTaskQueue.h
    core/source/SourceIndex.cpp
    core/source/SourceIndex.h
    core/verify/VerificationResultParser.cpp
    core/verify/VerificationResultParser.h
    core/verify/TestgridArtifactService.cpp
    core/verify/TestgridArtifactService.h
    core/verify/TestgridResultWriter.cpp
    core/verify/TestgridResultWriter.h
    core/verify/VerificationReportWriter.cpp
    core/verify/VerificationReportWriter.h
    core/verify/TestdiffArtifactAnalysis.cpp
    core/verify/TestdiffArtifactAnalysis.h
    core/verify/TestdiffGenerationContract.cpp
    core/verify/TestdiffGenerationContract.h
    core/verify/TestdiffGenerationPolicy.cpp
    core/verify/TestdiffGenerationPolicy.h
    core/verify/TestdiffGenerationResultWriter.cpp
    core/verify/TestdiffGenerationResultWriter.h
    core/verify/TestdiffArtifactIndex.cpp
    core/verify/TestdiffArtifactIndex.h
    core/verify/TestdiffArtifactScanner.cpp
    core/verify/TestdiffArtifactScanner.h
    core/verify/TestdiffAdapterResultWriter.cpp
    core/verify/TestdiffAdapterResultWriter.h
    core/verify/TestdiffCommandPlanner.cpp
    core/verify/TestdiffCommandPlanner.h
    core/verify/TestdiffRunnerAdapter.cpp
    core/verify/TestdiffRunnerAdapter.h
    core/verify/VerificationWorkflow.cpp
    core/verify/VerificationWorkflow.h
    core/verify/TwoStageFinalResultBuilder.cpp
    core/verify/TwoStageFinalResultBuilder.h
    core/verify/TwoStageFinalResultWriter.cpp
    core/verify/TwoStageFinalResultWriter.h
    core/verify/TwoStagePhaseResultWriter.cpp
    core/verify/TwoStagePhaseResultWriter.h
    core/verify/TwoStageTaskPlan.cpp
    core/verify/TwoStageTaskPlan.h
    core/verify/TwoStageVerificationResultWriter.cpp
    core/verify/TwoStageVerificationResultWriter.h
    workbench/CaseManifestSync.cpp
    workbench/CaseManifestSync.h
    workbench/CasePanel.cpp
    workbench/CasePanel.h
    workbench/DiffArtifactsPresenter.cpp
    workbench/DiffArtifactsPresenter.h
    workbench/DiffPanel.cpp
    workbench/DiffPanel.h
    workbench/EvidenceCoordinator.cpp
    workbench/EvidenceCoordinator.h
    workbench/EvidencePanel.cpp
    workbench/EvidencePanel.h
    workbench/ReportRefreshCoordinator.cpp
    workbench/ReportRefreshCoordinator.h
    workbench/SourcePanel.cpp
    workbench/SourcePanel.h
    workbench/TaskHistoryCoordinator.cpp
    workbench/TaskHistoryCoordinator.h
    workbench/TestdiffAdapterResultCoordinator.cpp
    workbench/TestdiffAdapterResultCoordinator.h
    workbench/TestgridTablePresenter.cpp
    workbench/TestgridTablePresenter.h
    workbench/TaskHistoryPanel.cpp
    workbench/TaskHistoryPanel.h
    workbench/TwoStageFinalResultCoordinator.cpp
    workbench/TwoStageFinalResultCoordinator.h
    workbench/TwoStageFinalResultUiAdapter.cpp
    workbench/TwoStageFinalResultUiAdapter.h
    workbench/VerificationPanel.cpp
    workbench/VerificationPanel.h
    workbench/WorkbenchMockData.cpp
    workbench/WorkbenchMockData.h
    workbench/WorkbenchWindow.cpp
    workbench/WorkbenchWindow.h
)

qt_add_executable(OCCTDebug
    ${OCCTDEBUG_SOURCES}
)

set_target_properties(OCCTDebug PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(OCCTDebug PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
)

target_compile_definitions(OCCTDebug PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    OCCTDEBUG_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
    OCCTDEBUG_BINARY_DIR="${CMAKE_BINARY_DIR}"
)

target_link_libraries(OCCTDebug PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OCCT_LIBS}
    ${FREETYPE_LIBS}
)

if(WIN32)
    target_link_libraries(OCCTDebug PRIVATE opengl32)
endif()

if(MSVC)
    target_compile_options(OCCTDebug PRIVATE /W4 /permissive-)
else()
    target_compile_options(OCCTDebug PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32 AND OCCTDEBUG_EFFECTIVE_QT_ROOT)
    find_program(OCCTDEBUG_WINDEPLOYQT
        NAMES windeployqt
        HINTS "${OCCTDEBUG_EFFECTIVE_QT_ROOT}/bin"
        NO_DEFAULT_PATH)

    if(OCCTDEBUG_WINDEPLOYQT)
        add_custom_command(
            TARGET OCCTDebug
            POST_BUILD
            COMMAND "${OCCTDEBUG_WINDEPLOYQT}"
                --no-translations
                --no-compiler-runtime
                "$<TARGET_FILE:OCCTDebug>"
            VERBATIM)
    else()
        add_custom_command(
            TARGET OCCTDebug
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${OCCTDEBUG_EFFECTIVE_QT_ROOT}/bin"
                "$<TARGET_FILE_DIR:OCCTDebug>"
            COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:OCCTDebug>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${OCCTDEBUG_EFFECTIVE_QT_ROOT}/plugins/platforms"
                "$<TARGET_FILE_DIR:OCCTDebug>/platforms"
            VERBATIM)
    endif()
endif()

if(WIN32 AND OCCT_DLLS)
    add_custom_command(
        TARGET OCCTDebug
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OCCT_DLLS}
            "$<TARGET_FILE_DIR:OCCTDebug>"
        VERBATIM)
endif()

if(WIN32 AND FREETYPE_DLLS)
    add_custom_command(
        TARGET OCCTDebug
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FREETYPE_DLLS}
            "$<TARGET_FILE_DIR:OCCTDebug>"
        VERBATIM)
endif()

if(WIN32 AND TCLTK_DLLS)
    add_custom_command(
        TARGET OCCTDebug
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${TCLTK_DLLS}
            "$<TARGET_FILE_DIR:OCCTDebug>"
            VERBATIM)
endif()

if(BUILD_TESTING)
    add_executable(OCCTDebugEvidenceBundleSmoke
        "${CMAKE_SOURCE_DIR}/tests/evidence_bundle_writer_smoke.cpp"
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
        core/evidence/EvidenceBundleWriter.cpp
        core/evidence/EvidenceBundleWriter.h
        core/geometry/TopologyCompareArtifact.cpp
        core/geometry/TopologyCompareArtifact.h
        core/geometry/TopologySignature.cpp
        core/geometry/TopologySignature.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugEvidenceBundleSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugEvidenceBundleSmoke PRIVATE Qt6::Core)
    target_link_libraries(OCCTDebugEvidenceBundleSmoke PRIVATE ${OCCT_LIBS})
    target_compile_definitions(OCCTDebugEvidenceBundleSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugEvidenceBundleSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugEvidenceBundleSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME evidence_bundle_smoke
        COMMAND $<TARGET_FILE:OCCTDebugEvidenceBundleSmoke>
            "${CMAKE_SOURCE_DIR}/sample_cases/OCC-LOCAL-2026-0001/case.json"
            "${CMAKE_BINARY_DIR}/test_artifacts/evidence_bundle_smoke/evidence_bundle.json"
    )
    set_tests_properties(evidence_bundle_smoke PROPERTIES
        LABELS "evidence;smoke;case"
        PASS_REGULAR_EXPRESSION "EVIDENCE_BUNDLE_SMOKE_OK"
    )

    add_executable(OCCTDebugVerificationReportSmoke
        "${CMAKE_SOURCE_DIR}/tests/verification_report_writer_smoke.cpp"
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
        core/geometry/TopologyCompareArtifact.cpp
        core/geometry/TopologyCompareArtifact.h
        core/geometry/TopologySignature.cpp
        core/geometry/TopologySignature.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
        core/verify/VerificationReportWriter.cpp
        core/verify/VerificationReportWriter.h
    )
    target_include_directories(OCCTDebugVerificationReportSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugVerificationReportSmoke PRIVATE Qt6::Core)
    target_link_libraries(OCCTDebugVerificationReportSmoke PRIVATE ${OCCT_LIBS})
    target_compile_definitions(OCCTDebugVerificationReportSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugVerificationReportSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugVerificationReportSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME verification_report_smoke
        COMMAND $<TARGET_FILE:OCCTDebugVerificationReportSmoke>
            "${CMAKE_SOURCE_DIR}/sample_cases/OCC-LOCAL-2026-0001/case.json"
            "${CMAKE_BINARY_DIR}/test_artifacts/verification_report_smoke/verification_report.md"
            "${CMAKE_BINARY_DIR}/test_artifacts/verification_report_smoke/verification_report.json"
    )
    set_tests_properties(verification_report_smoke PROPERTIES
        LABELS "verify;report;smoke;case"
        PASS_REGULAR_EXPRESSION "VERIFICATION_REPORT_SMOKE_OK"
    )

    add_executable(OCCTDebugReportRefreshCoordinatorSmoke
        "${CMAKE_SOURCE_DIR}/tests/report_refresh_coordinator_smoke.cpp"
        workbench/ReportRefreshCoordinator.cpp
        workbench/ReportRefreshCoordinator.h
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
        core/evidence/EvidenceBundleWriter.cpp
        core/evidence/EvidenceBundleWriter.h
        core/geometry/TopologyCompareArtifact.cpp
        core/geometry/TopologyCompareArtifact.h
        core/geometry/TopologySignature.cpp
        core/geometry/TopologySignature.h
        core/verify/VerificationReportWriter.cpp
        core/verify/VerificationReportWriter.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE Qt6::Core)
    target_link_libraries(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE ${OCCT_LIBS})
    target_compile_definitions(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugReportRefreshCoordinatorSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME report_refresh_coordinator_smoke
        COMMAND $<TARGET_FILE:OCCTDebugReportRefreshCoordinatorSmoke>
    )
    set_tests_properties(report_refresh_coordinator_smoke PROPERTIES
        LABELS "workbench;report;evidence;verify;smoke"
        PASS_REGULAR_EXPRESSION "REPORT_REFRESH_COORDINATOR_SMOKE_OK"
    )

    add_executable(OCCTDebugMarkdownReportGeneratorSmoke
        "${CMAKE_SOURCE_DIR}/tests/markdown_report_generator_smoke.cpp"
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
        core/report/MarkdownReportGenerator.cpp
        core/report/MarkdownReportGenerator.h
    )
    target_include_directories(OCCTDebugMarkdownReportGeneratorSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugMarkdownReportGeneratorSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugMarkdownReportGeneratorSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugMarkdownReportGeneratorSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugMarkdownReportGeneratorSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME markdown_report_generator_smoke
        COMMAND $<TARGET_FILE:OCCTDebugMarkdownReportGeneratorSmoke>
    )
    set_tests_properties(markdown_report_generator_smoke PROPERTIES
        LABELS "report;case;input;smoke"
        PASS_REGULAR_EXPRESSION "MARKDOWN_REPORT_GENERATOR_SMOKE_OK"
    )

    add_executable(OCCTDebugCppReproTemplateWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/cpp_repro_template_writer_smoke.cpp"
        core/repro/CppReproTemplateWriter.cpp
        core/repro/CppReproTemplateWriter.h
    )
    target_include_directories(OCCTDebugCppReproTemplateWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCppReproTemplateWriterSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCppReproTemplateWriterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCppReproTemplateWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCppReproTemplateWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME cpp_repro_template_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCppReproTemplateWriterSmoke>
    )
    set_tests_properties(cpp_repro_template_writer_smoke PROPERTIES
        LABELS "repro;cpp;case;smoke"
        PASS_REGULAR_EXPRESSION "CPP_REPRO_TEMPLATE_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugReproStatusEvaluatorSmoke
        "${CMAKE_SOURCE_DIR}/tests/repro_status_evaluator_smoke.cpp"
        core/repro/ReproStatusEvaluator.cpp
        core/repro/ReproStatusEvaluator.h
        core/repro/CppReproTemplateWriter.h
        core/repro/DrawLogParser.h
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
    )
    target_include_directories(OCCTDebugReproStatusEvaluatorSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugReproStatusEvaluatorSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugReproStatusEvaluatorSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugReproStatusEvaluatorSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugReproStatusEvaluatorSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME repro_status_evaluator_smoke
        COMMAND $<TARGET_FILE:OCCTDebugReproStatusEvaluatorSmoke>
    )
    set_tests_properties(repro_status_evaluator_smoke PROPERTIES
        LABELS "repro;status;case;smoke"
        PASS_REGULAR_EXPRESSION "REPRO_STATUS_EVALUATOR_SMOKE_OK"
    )

    add_executable(OCCTDebugVerificationWorkflowSmoke
        "${CMAKE_SOURCE_DIR}/tests/verification_workflow_smoke.cpp"
        core/verify/VerificationWorkflow.cpp
        core/verify/VerificationWorkflow.h
    )
    target_include_directories(OCCTDebugVerificationWorkflowSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugVerificationWorkflowSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugVerificationWorkflowSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugVerificationWorkflowSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugVerificationWorkflowSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME verification_workflow_smoke
        COMMAND $<TARGET_FILE:OCCTDebugVerificationWorkflowSmoke>
    )
    set_tests_properties(verification_workflow_smoke PROPERTIES
        LABELS "verify;workflow;smoke"
        PASS_REGULAR_EXPRESSION "VERIFICATION_WORKFLOW_SMOKE_OK"
    )

    add_executable(OCCTDebugCommandRunnerCancelSmoke
        "${CMAKE_SOURCE_DIR}/tests/command_runner_cancel_smoke.cpp"
        core/runner/CommandRunner.cpp
        core/runner/CommandRunner.h
    )
    target_include_directories(OCCTDebugCommandRunnerCancelSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCommandRunnerCancelSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCommandRunnerCancelSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCommandRunnerCancelSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCommandRunnerCancelSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME command_runner_cancel_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCommandRunnerCancelSmoke>
    )
    set_tests_properties(command_runner_cancel_smoke PROPERTIES
        LABELS "runner;cancel;smoke"
        PASS_REGULAR_EXPRESSION "COMMAND_RUNNER_CANCEL_SMOKE_OK"
    )

    add_executable(OCCTDebugCommandRunnerTimeoutSmoke
        "${CMAKE_SOURCE_DIR}/tests/command_runner_timeout_smoke.cpp"
        core/runner/CommandRunner.cpp
        core/runner/CommandRunner.h
    )
    target_include_directories(OCCTDebugCommandRunnerTimeoutSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCommandRunnerTimeoutSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCommandRunnerTimeoutSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCommandRunnerTimeoutSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCommandRunnerTimeoutSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME command_runner_timeout_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCommandRunnerTimeoutSmoke>
    )
    set_tests_properties(command_runner_timeout_smoke PROPERTIES
        LABELS "runner;timeout;smoke"
        PASS_REGULAR_EXPRESSION "COMMAND_RUNNER_TIMEOUT_SMOKE_OK"
    )

    add_executable(OCCTDebugCommandTaskQueueSmoke
        "${CMAKE_SOURCE_DIR}/tests/command_task_queue_smoke.cpp"
        core/runner/CommandRunner.cpp
        core/runner/CommandRunner.h
        core/runner/CommandTaskQueue.cpp
        core/runner/CommandTaskQueue.h
    )
    target_include_directories(OCCTDebugCommandTaskQueueSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCommandTaskQueueSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCommandTaskQueueSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCommandTaskQueueSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCommandTaskQueueSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME command_task_queue_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCommandTaskQueueSmoke>
    )
    set_tests_properties(command_task_queue_smoke PROPERTIES
        LABELS "runner;queue;cancel;dry-run;smoke"
        PASS_REGULAR_EXPRESSION "COMMAND_TASK_QUEUE_SMOKE_OK"
    )

    add_executable(OCCTDebugVerificationResultParserSmoke
        "${CMAKE_SOURCE_DIR}/tests/verification_result_parser_smoke.cpp"
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugVerificationResultParserSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugVerificationResultParserSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugVerificationResultParserSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugVerificationResultParserSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugVerificationResultParserSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME verification_result_parser_smoke
        COMMAND $<TARGET_FILE:OCCTDebugVerificationResultParserSmoke>
    )
    set_tests_properties(verification_result_parser_smoke PROPERTIES
        LABELS "verify;parser;smoke"
        PASS_REGULAR_EXPRESSION "VERIFICATION_RESULT_PARSER_SMOKE_OK"
    )

    add_executable(OCCTDebugWorkbenchPresenterSmoke
        "${CMAKE_SOURCE_DIR}/tests/workbench_presenter_smoke.cpp"
        workbench/CaseManifestSync.cpp
        workbench/CaseManifestSync.h
        workbench/DiffArtifactsPresenter.cpp
        workbench/DiffArtifactsPresenter.h
        workbench/DiffPanel.cpp
        workbench/DiffPanel.h
        workbench/EvidenceCoordinator.cpp
        workbench/EvidenceCoordinator.h
        workbench/EvidencePanel.cpp
        workbench/EvidencePanel.h
        workbench/TestdiffAdapterResultCoordinator.cpp
        workbench/TestdiffAdapterResultCoordinator.h
        workbench/TestgridTablePresenter.cpp
        workbench/TestgridTablePresenter.h
        workbench/TaskHistoryCoordinator.cpp
        workbench/TaskHistoryCoordinator.h
        workbench/TaskHistoryPanel.cpp
        workbench/TaskHistoryPanel.h
        workbench/TwoStageFinalResultCoordinator.cpp
        workbench/TwoStageFinalResultCoordinator.h
        workbench/TwoStageFinalResultUiAdapter.cpp
        workbench/TwoStageFinalResultUiAdapter.h
        workbench/VerificationPanel.cpp
        workbench/VerificationPanel.h
        core/verify/TestdiffAdapterResultWriter.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugWorkbenchPresenterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugWorkbenchPresenterSmoke PRIVATE Qt6::Core Qt6::Widgets)
    target_compile_definitions(OCCTDebugWorkbenchPresenterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugWorkbenchPresenterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugWorkbenchPresenterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME workbench_presenter_smoke
        COMMAND $<TARGET_FILE:OCCTDebugWorkbenchPresenterSmoke>
    )
    set_tests_properties(workbench_presenter_smoke PROPERTIES
        LABELS "workbench;presenter;evidence;testgrid;smoke"
        PASS_REGULAR_EXPRESSION "WORKBENCH_PRESENTER_SMOKE_OK"
    )

    add_executable(OCCTDebugCaseManifestPlanSmoke
        "${CMAKE_SOURCE_DIR}/tests/case_manifest_plan_smoke.cpp"
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
    )
    target_include_directories(OCCTDebugCaseManifestPlanSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCaseManifestPlanSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCaseManifestPlanSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCaseManifestPlanSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCaseManifestPlanSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME case_manifest_plan_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCaseManifestPlanSmoke>
            "${CMAKE_SOURCE_DIR}/sample_cases/OCC-LOCAL-2026-0001/case.json"
    )
    set_tests_properties(case_manifest_plan_smoke PROPERTIES
        LABELS "case;manifest;smoke"
        PASS_REGULAR_EXPRESSION "CASE_MANIFEST_PLAN_SMOKE_OK"
    )

    add_executable(OCCTDebugCrashDumpArchiveSmoke
        "${CMAKE_SOURCE_DIR}/tests/crash_dump_archive_smoke.cpp"
        core/case/CrashDumpArchive.cpp
        core/case/CrashDumpArchive.h
    )
    target_include_directories(OCCTDebugCrashDumpArchiveSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugCrashDumpArchiveSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugCrashDumpArchiveSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugCrashDumpArchiveSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugCrashDumpArchiveSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME crash_dump_archive_smoke
        COMMAND $<TARGET_FILE:OCCTDebugCrashDumpArchiveSmoke>
    )
    set_tests_properties(crash_dump_archive_smoke PROPERTIES
        LABELS "case;crash;artifact;smoke"
        PASS_REGULAR_EXPRESSION "CRASH_DUMP_ARCHIVE_SMOKE_OK"
    )

    add_executable(OCCTDebugTestgridArtifactServiceSmoke
        "${CMAKE_SOURCE_DIR}/tests/testgrid_artifact_service_smoke.cpp"
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugTestgridArtifactServiceSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestgridArtifactServiceSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestgridArtifactServiceSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestgridArtifactServiceSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestgridArtifactServiceSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testgrid_artifact_service_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestgridArtifactServiceSmoke>
    )
    set_tests_properties(testgrid_artifact_service_smoke PROPERTIES
        LABELS "verify;testgrid;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TESTGRID_ARTIFACT_SERVICE_SMOKE_OK"
    )

    add_executable(OCCTDebugTestgridResultWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/testgrid_result_writer_smoke.cpp"
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
        core/verify/TestgridResultWriter.cpp
        core/verify/TestgridResultWriter.h
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/TwoStageVerificationResultWriter.cpp
        core/verify/TwoStageVerificationResultWriter.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugTestgridResultWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestgridResultWriterSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestgridResultWriterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestgridResultWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestgridResultWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testgrid_result_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestgridResultWriterSmoke>
    )
    set_tests_properties(testgrid_result_writer_smoke PROPERTIES
        LABELS "verify;testgrid;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TESTGRID_RESULT_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffCommandPlannerSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_command_planner_smoke.cpp"
        core/verify/TestdiffCommandPlanner.cpp
        core/verify/TestdiffCommandPlanner.h
    )
    target_include_directories(OCCTDebugTestdiffCommandPlannerSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffCommandPlannerSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffCommandPlannerSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffCommandPlannerSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffCommandPlannerSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_command_planner_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffCommandPlannerSmoke>
    )
    set_tests_properties(testdiff_command_planner_smoke PROPERTIES
        LABELS "verify;testdiff;runner;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_COMMAND_PLANNER_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffRunnerAdapterSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_runner_adapter_smoke.cpp"
        core/verify/TestdiffRunnerAdapter.cpp
        core/verify/TestdiffRunnerAdapter.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
    )
    target_include_directories(OCCTDebugTestdiffRunnerAdapterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffRunnerAdapterSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffRunnerAdapterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffRunnerAdapterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffRunnerAdapterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_runner_adapter_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffRunnerAdapterSmoke>
    )
    set_tests_properties(testdiff_runner_adapter_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_RUNNER_ADAPTER_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffArtifactIndexSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_artifact_index_smoke.cpp"
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
    )
    target_include_directories(OCCTDebugTestdiffArtifactIndexSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffArtifactIndexSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffArtifactIndexSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffArtifactIndexSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffArtifactIndexSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_artifact_index_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffArtifactIndexSmoke>
    )
    set_tests_properties(testdiff_artifact_index_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_ARTIFACT_INDEX_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffArtifactAnalysisSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_artifact_analysis_smoke.cpp"
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
    )
    target_include_directories(OCCTDebugTestdiffArtifactAnalysisSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffArtifactAnalysisSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffArtifactAnalysisSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffArtifactAnalysisSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffArtifactAnalysisSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_artifact_analysis_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffArtifactAnalysisSmoke>
    )
    set_tests_properties(testdiff_artifact_analysis_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;analysis;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_ARTIFACT_ANALYSIS_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffGenerationPolicySmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_generation_policy_smoke.cpp"
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
    )
    target_include_directories(OCCTDebugTestdiffGenerationPolicySmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffGenerationPolicySmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffGenerationPolicySmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffGenerationPolicySmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffGenerationPolicySmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_generation_policy_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffGenerationPolicySmoke>
    )
    set_tests_properties(testdiff_generation_policy_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;generation;policy;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_GENERATION_POLICY_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffGenerationContractSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_generation_contract_smoke.cpp"
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
    )
    target_include_directories(OCCTDebugTestdiffGenerationContractSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffGenerationContractSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffGenerationContractSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffGenerationContractSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffGenerationContractSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_generation_contract_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffGenerationContractSmoke>
    )
    set_tests_properties(testdiff_generation_contract_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;generation;contract;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_GENERATION_CONTRACT_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffGenerationFailureReportSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_generation_failure_report_smoke.cpp"
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
    )
    target_include_directories(OCCTDebugTestdiffGenerationFailureReportSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffGenerationFailureReportSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffGenerationFailureReportSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffGenerationFailureReportSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffGenerationFailureReportSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_generation_failure_report_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffGenerationFailureReportSmoke>
    )
    set_tests_properties(testdiff_generation_failure_report_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;generation;failure;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_GENERATION_FAILURE_REPORT_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffGenerationResultWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_generation_result_writer_smoke.cpp"
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffGenerationResultWriter.cpp
        core/verify/TestdiffGenerationResultWriter.h
    )
    target_include_directories(OCCTDebugTestdiffGenerationResultWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffGenerationResultWriterSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffGenerationResultWriterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffGenerationResultWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffGenerationResultWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_generation_result_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffGenerationResultWriterSmoke>
    )
    set_tests_properties(testdiff_generation_result_writer_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;generation;writer;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_GENERATION_RESULT_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugTestdiffAdapterResultWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/testdiff_adapter_result_writer_smoke.cpp"
        core/verify/TestdiffAdapterResultWriter.cpp
        core/verify/TestdiffAdapterResultWriter.h
        core/verify/TestdiffRunnerAdapter.cpp
        core/verify/TestdiffRunnerAdapter.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugTestdiffAdapterResultWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTestdiffAdapterResultWriterSmoke PRIVATE Qt6::Core)
    target_compile_definitions(OCCTDebugTestdiffAdapterResultWriterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTestdiffAdapterResultWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTestdiffAdapterResultWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME testdiff_adapter_result_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTestdiffAdapterResultWriterSmoke>
    )
    set_tests_properties(testdiff_adapter_result_writer_smoke PROPERTIES
        LABELS "verify;testdiff;artifact;writer;smoke"
        PASS_REGULAR_EXPRESSION "TESTDIFF_ADAPTER_RESULT_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugTwoStagePhaseResultWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/two_stage_phase_result_writer_smoke.cpp"
        core/verify/TwoStagePhaseResultWriter.cpp
        core/verify/TwoStagePhaseResultWriter.h
        core/verify/TwoStageVerificationResultWriter.cpp
        core/verify/TwoStageVerificationResultWriter.h
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
    )
    target_include_directories(OCCTDebugTwoStagePhaseResultWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTwoStagePhaseResultWriterSmoke PRIVATE
        Qt6::Core
    )
    target_compile_definitions(OCCTDebugTwoStagePhaseResultWriterSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTwoStagePhaseResultWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTwoStagePhaseResultWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME two_stage_phase_result_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTwoStagePhaseResultWriterSmoke>
    )
    set_tests_properties(two_stage_phase_result_writer_smoke PROPERTIES
        LABELS "verify;writer;testgrid;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TWO_STAGE_PHASE_RESULT_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugTwoStageFinalResultBuilderSmoke
        "${CMAKE_SOURCE_DIR}/tests/two_stage_final_result_builder_smoke.cpp"
        core/verify/TwoStageFinalResultBuilder.cpp
        core/verify/TwoStageFinalResultBuilder.h
        core/verify/TwoStageFinalResultWriter.cpp
        core/verify/TwoStageFinalResultWriter.h
        core/verify/TwoStageVerificationResultWriter.cpp
        core/verify/TwoStageVerificationResultWriter.h
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
        core/verify/VerificationWorkflow.cpp
        core/verify/VerificationWorkflow.h
    )
    target_include_directories(OCCTDebugTwoStageFinalResultBuilderSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTwoStageFinalResultBuilderSmoke PRIVATE
        Qt6::Core
    )
    target_compile_definitions(OCCTDebugTwoStageFinalResultBuilderSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTwoStageFinalResultBuilderSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTwoStageFinalResultBuilderSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME two_stage_final_result_builder_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTwoStageFinalResultBuilderSmoke>
    )
    set_tests_properties(two_stage_final_result_builder_smoke PROPERTIES
        LABELS "verify;builder;testgrid;artifact;smoke"
        PASS_REGULAR_EXPRESSION "TWO_STAGE_FINAL_RESULT_BUILDER_SMOKE_OK"
    )

    add_executable(OCCTDebugTwoStageTaskPlanSmoke
        "${CMAKE_SOURCE_DIR}/tests/two_stage_task_plan_smoke.cpp"
        core/verify/TwoStageTaskPlan.cpp
        core/verify/TwoStageTaskPlan.h
    )
    target_include_directories(OCCTDebugTwoStageTaskPlanSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTwoStageTaskPlanSmoke PRIVATE
        Qt6::Core
    )
    target_compile_definitions(OCCTDebugTwoStageTaskPlanSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTwoStageTaskPlanSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTwoStageTaskPlanSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME two_stage_task_plan_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTwoStageTaskPlanSmoke>
    )
    set_tests_properties(two_stage_task_plan_smoke PROPERTIES
        LABELS "verify;testgrid;workflow;queue;dry-run;smoke"
        PASS_REGULAR_EXPRESSION "TWO_STAGE_TASK_PLAN_SMOKE_OK"
    )

    add_executable(OCCTDebugTwoStageVerificationResultWriterSmoke
        "${CMAKE_SOURCE_DIR}/tests/two_stage_verification_result_writer_smoke.cpp"
        core/verify/TwoStageVerificationResultWriter.cpp
        core/verify/TwoStageVerificationResultWriter.h
        core/verify/TwoStageFinalResultWriter.cpp
        core/verify/TwoStageFinalResultWriter.h
        core/verify/TestgridArtifactService.cpp
        core/verify/TestgridArtifactService.h
        core/verify/TestdiffArtifactAnalysis.cpp
        core/verify/TestdiffArtifactAnalysis.h
        core/verify/TestdiffGenerationContract.cpp
        core/verify/TestdiffGenerationContract.h
        core/verify/TestdiffGenerationPolicy.cpp
        core/verify/TestdiffGenerationPolicy.h
        core/verify/TestdiffArtifactIndex.cpp
        core/verify/TestdiffArtifactIndex.h
        core/verify/TestdiffArtifactScanner.cpp
        core/verify/TestdiffArtifactScanner.h
        core/verify/VerificationResultParser.cpp
        core/verify/VerificationResultParser.h
        core/case/CaseManifest.cpp
        core/case/CaseManifest.h
    )
    target_include_directories(OCCTDebugTwoStageVerificationResultWriterSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTwoStageVerificationResultWriterSmoke PRIVATE
        Qt6::Core
    )
    if(MSVC)
        target_compile_options(OCCTDebugTwoStageVerificationResultWriterSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTwoStageVerificationResultWriterSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME two_stage_verification_result_writer_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTwoStageVerificationResultWriterSmoke>
    )
    set_tests_properties(two_stage_verification_result_writer_smoke PROPERTIES
        LABELS "verify;writer;smoke"
        PASS_REGULAR_EXPRESSION "TWO_STAGE_VERIFICATION_RESULT_WRITER_SMOKE_OK"
    )

    add_executable(OCCTDebugTopologySignatureSmoke
        "${CMAKE_SOURCE_DIR}/tests/topology_signature_smoke.cpp"
        core/geometry/TopologySignature.cpp
        core/geometry/TopologySignature.h
    )
    target_include_directories(OCCTDebugTopologySignatureSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTopologySignatureSmoke PRIVATE
        Qt6::Core
        ${OCCT_LIBS}
        ${FREETYPE_LIBS}
    )
    target_compile_definitions(OCCTDebugTopologySignatureSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTopologySignatureSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTopologySignatureSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME topology_signature_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTopologySignatureSmoke>
    )
    set_tests_properties(topology_signature_smoke PROPERTIES
        LABELS "geometry;topology;smoke;occt"
        PASS_REGULAR_EXPRESSION "TOPOLOGY_SIGNATURE_SMOKE_OK"
    )

    add_executable(OCCTDebugTopologyCompareArtifactSmoke
        "${CMAKE_SOURCE_DIR}/tests/topology_compare_artifact_smoke.cpp"
        core/geometry/TopologyCompareArtifact.cpp
        core/geometry/TopologyCompareArtifact.h
        core/geometry/TopologySignature.cpp
        core/geometry/TopologySignature.h
    )
    target_include_directories(OCCTDebugTopologyCompareArtifactSmoke PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
    )
    target_link_libraries(OCCTDebugTopologyCompareArtifactSmoke PRIVATE
        Qt6::Core
        ${OCCT_LIBS}
        ${FREETYPE_LIBS}
    )
    target_compile_definitions(OCCTDebugTopologyCompareArtifactSmoke PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    if(MSVC)
        target_compile_options(OCCTDebugTopologyCompareArtifactSmoke PRIVATE /W4 /permissive-)
    else()
        target_compile_options(OCCTDebugTopologyCompareArtifactSmoke PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    add_test(
        NAME topology_compare_artifact_smoke
        COMMAND $<TARGET_FILE:OCCTDebugTopologyCompareArtifactSmoke>
    )
    set_tests_properties(topology_compare_artifact_smoke PROPERTIES
        LABELS "geometry;topology;artifact;smoke;occt"
        PASS_REGULAR_EXPRESSION "TOPOLOGY_COMPARE_ARTIFACT_SMOKE_OK"
    )
endif()
