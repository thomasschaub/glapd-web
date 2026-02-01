add_library(parpl src/par.cpp)
target_include_directories(parpl PUBLIC src)
target_link_libraries(parpl PRIVATE bowtie-wrapper)