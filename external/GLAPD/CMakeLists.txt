add_library(glapd
    GLAPD/LAMP.c
    GLAPD/single.c
)

target_compile_options(glapd PRIVATE -w)
