file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
add_executable(portable-glapd ${srcs})
target_link_libraries(portable-glapd PRIVATE bowtie-wrapper glapd minizip parpl)

if(EMSCRIPTEN)
    # Embed par dir
    target_link_options(portable-glapd PRIVATE
        --js-library=${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js
        --embed-file=${CMAKE_SOURCE_DIR}/external/glapd/GLAPD/Par@external/glapd/GLAPD/Par)

    set_property(TARGET glapd APPEND PROPERTY
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js)

    install(
        FILES
            "$<TARGET_FILE_DIR:portable-glapd>/portable-glapd.js"
            "$<TARGET_FILE_DIR:portable-glapd>/portable-glapd.wasm"
        DESTINATION .
    )
endif()
