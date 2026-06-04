# Qt6: include local defaults first, then CMAKE_PREFIX_PATH / cache (aligns with MovementPath usage).

set(_occtdebug_qt_defaults "${CMAKE_CURRENT_LIST_DIR}/QtWorkbenchDefaults.cmake")
if(EXISTS "${_occtdebug_qt_defaults}")
    include("${_occtdebug_qt_defaults}")
endif()

if(NOT OCCTDEBUG_QT_DEFAULT_KIT AND DEFINED MOVEMENTPATH_QT_WORKBENCH_DEFAULT_KIT)
    set(OCCTDEBUG_QT_DEFAULT_KIT "${MOVEMENTPATH_QT_WORKBENCH_DEFAULT_KIT}")
endif()
if(NOT OCCTDEBUG_QT_DEFAULT_KIT)
    set(OCCTDEBUG_QT_DEFAULT_KIT "C:/Qt/6.11.0/msvc2022_64")
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
    core/case/CaseManifest.cpp
    core/case/CaseManifest.h
    core/geometry/OcctViewerWidget.cpp
    core/geometry/OcctViewerWidget.h
    core/knowledge/SimilarCaseSearch.cpp
    core/knowledge/SimilarCaseSearch.h
    core/Logger.cpp
    core/Logger.h
    core/patch/PatchReview.cpp
    core/patch/PatchReview.h
    core/report/MarkdownReportGenerator.cpp
    core/report/MarkdownReportGenerator.h
    core/runner/CommandRunner.cpp
    core/runner/CommandRunner.h
    core/source/SourceIndex.cpp
    core/source/SourceIndex.h
    core/verify/VerificationResultParser.cpp
    core/verify/VerificationResultParser.h
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
