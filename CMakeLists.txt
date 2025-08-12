cmake_minimum_required(VERSION 4.0)
project(glapd-web)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(external)

add_executable(glapd-web
    src/par.cpp
    src/main.cpp
)
target_link_libraries(glapd-web PRIVATE bowtie glapd)

if(EMSCRIPTEN)
    # Embed par dir
    target_link_options(glapd-web PRIVATE
        --embed-file=${CMAKE_CURRENT_LIST_DIR}/external/glapd/GLAPD/Par@external/glapd/GLAPD/Par)

    install(
        FILES
            "$<TARGET_FILE_DIR:glapd-web>/glapd-web.js"
            "$<TARGET_FILE_DIR:glapd-web>/glapd-web.wasm"
        DESTINATION .
    )
endif()
