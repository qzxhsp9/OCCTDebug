set(FREETYPE_INSTALL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/occt_3rdparty/freetype-2.13.3-x64")
set(TCLTK_INSTALL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/occt_3rdparty/tcltk-8.6.15-x64")

include_directories(${FREETYPE_INSTALL_PATH}/include)
include_directories(${TCLTK_INSTALL_PATH}/include)

link_directories(${FREETYPE_INSTALL_PATH})
link_directories(${TCLTK_INSTALL_PATH}/lib)
file(GLOB FREETYPE_LIBS ${FREETYPE_INSTALL_PATH}/lib/*.lib)
file(GLOB FREETYPE_DLLS ${FREETYPE_INSTALL_PATH}/bin/*.dll)
file(GLOB TCLTK_LIBS ${TCLTK_INSTALL_PATH}/lib/*.lib)
file(GLOB TCLTK_DLLS ${TCLTK_INSTALL_PATH}/bin/*.dll)
