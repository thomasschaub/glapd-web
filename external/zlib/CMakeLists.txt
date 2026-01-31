add_library(minizip
    zlib/contrib/minizip/unzip.c
    zlib/contrib/minizip/zip.c
    zlib/contrib/minizip/ioapi.c
    zlib/contrib/minizip/mztools.c
    #zlib/contrib/minizip/iowin32.c
    zlib/contrib/minizip/minizip.c
    zlib/contrib/minizip/miniunz.c
)

target_include_directories(minizip PUBLIC zlib/contrib/minizip)
target_link_libraries(minizip PUBLIC z)