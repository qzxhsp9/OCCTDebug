# Qt6: prefer existing CMAKE_PREFIX_PATH / Qt6_DIR (e.g. VS CMake settings), then local defaults.

find_package(Qt6 6.5 QUIET COMPONENTS Core Gui Widgets)

if(NOT Qt6_FOUND)
    set(_occtdebug_qt_defaults "${CMAKE_CURRENT_LIST_DIR}/QtWorkbenchDefaults.cmake")
    if(EXISTS "${_occtdebug_qt_defaults}")
        include("${_occtdebug_qt_defaults}")
    endif()
    if(NOT OCCTDEBUG_QT_DEFAULT_KIT)
        set(OCCTDEBUG_QT_DEFAULT_KIT "C:/Qt/6.11.0/msvc2022_64")
    endif()

    set(OCCTDEBUG_QT_ROOT
        ""
        CACHE PATH
        "Qt6 kit root (directory containing lib/cmake/Qt6). See QtWorkbenchDefaults.cmake.example.")

    if(NOT OCCTDEBUG_QT_ROOT AND
        EXISTS "${OCCTDEBUG_QT_DEFAULT_KIT}/lib/cmake/Qt6/Qt6Config.cmake")
        set(OCCTDEBUG_QT_ROOT
            "${OCCTDEBUG_QT_DEFAULT_KIT}"
            CACHE PATH
            "Qt6 kit root (directory containing lib/cmake/Qt6). See QtWorkbenchDefaults.cmake.example."
            FORCE)
    endif()

    if(OCCTDEBUG_QT_ROOT)
        list(FIND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_ROOT}" _occtdebug_qt_prefix_idx)
        if(_occtdebug_qt_prefix_idx LESS 0)
            list(PREPEND CMAKE_PREFIX_PATH "${OCCTDEBUG_QT_ROOT}")
        endif()
    endif()

    find_package(Qt6 6.5 QUIET COMPONENTS Core Gui Widgets)
endif()

if(NOT Qt6_FOUND)
    message(FATAL_ERROR
        "OCCTDebug: Qt6 (>= 6.5, Core/Gui/Widgets) was not found.\n"
        "Set one of the following, then re-configure:\n"
        "  - OCCTDEBUG_QT_ROOT = path to the kit (folder that contains lib/cmake/Qt6)\n"
        "  - CMAKE_PREFIX_PATH including that same kit root (Visual Studio: CMake variables / CMakeSettings)\n"
        "  - Qt6_DIR = .../lib/cmake/Qt6\n"
        "Or copy src/QtWorkbenchDefaults.cmake.example to src/QtWorkbenchDefaults.cmake and set OCCTDEBUG_QT_DEFAULT_KIT.\n"
        "Current OCCTDEBUG_QT_ROOT='${OCCTDEBUG_QT_ROOT}'\n"
        "CMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATH}'")
endif()

# For windeployqt when Qt was found only via CMAKE_PREFIX_PATH / Qt6_DIR.
set(OCCTDEBUG_EFFECTIVE_QT_ROOT "${OCCTDEBUG_QT_ROOT}")
if(NOT OCCTDEBUG_EFFECTIVE_QT_ROOT AND Qt6_DIR)
    get_filename_component(OCCTDEBUG_EFFECTIVE_QT_ROOT "${Qt6_DIR}/../../.." ABSOLUTE)
endif()

set(OCCTDEBUG_SOURCES
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

qt_add_executable(OCCTDebug
    ${OCCTDEBUG_SOURCES}
)

set_target_properties(OCCTDebug PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(OCCTDebug PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    ${OCCT_LIBS}
    ${FREETYPE_LIBS}
)

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
