
add_library(bowtie
    bowtie/alphabet.cpp
    #bowtie/bowtie_build_main.cpp
    #bowtie/bowtie_inspect.cpp
    #bowtie/bowtie_main.cpp
    bowtie/bt2_locks.cpp
    bowtie/ccnt_lut.cpp
    #bowtie/chaincat.cpp
    #bowtie/cpu_numa_info.cpp
    bowtie/ebwt.cpp
    bowtie/ebwt_build.cpp
    bowtie/ebwt_search.cpp
    bowtie/ebwt_search_util.cpp
    bowtie/edit.cpp
    bowtie/hit.cpp
    bowtie/hit_set.cpp
    bowtie/log.cpp
    #bowtie/ls.cpp
    bowtie/pat.cpp
    bowtie/qual.cpp
    #bowtie/random_test.cpp
    bowtie/ref_aligner.cpp
    bowtie/ref_read.cpp
    bowtie/sam.cpp
    bowtie/shmem.cpp
    bowtie/tinythread.cpp)

target_compile_definitions(bowtie PRIVATE
    BOWTIE_VERSION="v1.3.1"
    BUILD_HOST=""
    BUILD_TIME=""
    COMPILER_VERSION=""
    COMPILER_OPTIONS=""
)

target_compile_definitions(bowtie PRIVATE NDEBUG) # we get assertions, but its not like we're gonna do anything about them
target_compile_options(bowtie PRIVATE -w)         # same for warnings

if(EMSCRIPTEN)
    target_compile_options(bowtie PRIVATE "-sUSE_ZLIB=1")
    target_link_options(bowtie INTERFACE "-sUSE_ZLIB=1" "-sSTACK_SIZE=524288" "-sALLOW_MEMORY_GROWTH")
else()
    target_link_libraries(bowtie PRIVATE z)
endif()
