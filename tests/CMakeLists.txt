set(_occtdebug_smoke_sources
    "${CMAKE_SOURCE_DIR}/src/core/ShapeDocument.cpp"
    "${CMAKE_SOURCE_DIR}/src/core/ShapeKind.cpp"
    "${CMAKE_SOURCE_DIR}/src/occt/ShapeInspector.cpp")

add_executable(occtdebug_shape_smoke shape_smoke.cpp ${_occtdebug_smoke_sources})
target_include_directories(occtdebug_shape_smoke PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_link_libraries(occtdebug_shape_smoke PRIVATE ${OCCT_LIBS} ${FREETYPE_LIBS})

add_test(NAME occtdebug_shape_smoke COMMAND occtdebug_shape_smoke)

add_executable(
    occtdebug_problem_document_importer
    problem_document_importer.cpp
    "${CMAKE_SOURCE_DIR}/src/analysis/ProblemDocumentImporter.cpp")
target_include_directories(occtdebug_problem_document_importer PRIVATE "${CMAKE_SOURCE_DIR}/src")

add_test(NAME occtdebug_problem_document_importer COMMAND occtdebug_problem_document_importer)
