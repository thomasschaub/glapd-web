add_library(glapd
    GLAPD/LAMP.c
    GLAPD/single.c
)

target_compile_options(glapd PRIVATE -w)

if(EMSCRIPTEN)
    target_link_options(glapd INTERFACE --js-library ${CMAKE_CURRENT_SOURCE_DIR}/GLAPD/glapd_signals.js)
    set_property(TARGET glapd APPEND PROPERTY
        INTERFACE_LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/GLAPD/glapd_signals.js)
endif()