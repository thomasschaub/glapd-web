file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
add_executable(portable-glapd ${srcs})
target_link_libraries(portable-glapd PRIVATE bowtie-wrapper glapd minizip parpl)

if(EMSCRIPTEN)
    # Embed par dir
    target_link_options(portable-glapd PRIVATE
        --js-library=${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js
        --embed-file=${CMAKE_SOURCE_DIR}/external/glapd/GLAPD/Par@external/glapd/GLAPD/Par)

    set(CACHE{PORTABLE_GLAPD_INSTALL_BASENAME} TYPE STRING HELP "Filename without extension for the installed files" VALUE "portable-glapd")
    set_target_properties(portable-glapd PROPERTIES
        OUTPUT_NAME ${PORTABLE_GLAPD_INSTALL_BASENAME}
        )

    set_property(TARGET glapd APPEND PROPERTY
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js)

    install(FILES "$<TARGET_FILE_DIR:portable-glapd>/${PORTABLE_GLAPD_INSTALL_BASENAME}.js"   DESTINATION .)
    install(FILES "$<TARGET_FILE_DIR:portable-glapd>/${PORTABLE_GLAPD_INSTALL_BASENAME}.wasm" DESTINATION .)
endif()
