add_library(bowtie-wrapper INTERFACE)
target_include_directories(bowtie-wrapper INTERFACE include)
target_link_libraries(bowtie-wrapper INTERFACE bowtie)