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
    TKernel
)

if(MSVC)
    target_compile_options(OCCTDebug PRIVATE /W4 /permissive-)
else()
    target_compile_options(OCCTDebug PRIVATE -Wall -Wextra -Wpedantic)
endif()
