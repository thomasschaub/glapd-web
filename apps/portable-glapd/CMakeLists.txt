file(GLOB_RECURSE srcs CONFIGURE_DEPENDS src/*.cpp)
add_executable(glapd-web ${srcs})
target_link_libraries(glapd-web PRIVATE bowtie-wrapper glapd minizip parpl)

if(EMSCRIPTEN)
    # Embed par dir
    target_link_options(glapd-web PRIVATE
        --js-library=${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js
        --embed-file=${CMAKE_SOURCE_DIR}/external/glapd/GLAPD/Par@external/glapd/GLAPD/Par)

    set_property(TARGET glapd APPEND PROPERTY
        LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/signals.js)

    install(
        FILES
            "$<TARGET_FILE_DIR:glapd-web>/glapd-web.js"
            "$<TARGET_FILE_DIR:glapd-web>/glapd-web.wasm"
        DESTINATION .
    )
endif()
